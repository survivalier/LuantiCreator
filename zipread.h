#pragma once
// Lecture d'archives .zip autonome (C++ standard uniquement) : liste des fichiers, extraction
// (méthodes « stored » et « deflate »), détection des mods Luanti contenus dans l'archive.

#include <algorithm>
#include <climits>
#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "inflate.h"

namespace lczip {

inline uint32_t crc32(const std::string &d)
{
    static uint32_t table[256];
    static bool ready = false;
    if (!ready) {
        for (uint32_t i = 0; i < 256; ++i) {
            uint32_t c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        ready = true;
    }
    uint32_t c = 0xFFFFFFFFu;
    for (unsigned char b : d) c = table[(c ^ b) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

struct Entry {
    std::string name;
    uint32_t crc = 0, csize = 0, usize = 0, offset = 0;
    uint16_t method = 0, flags = 0;
};

inline uint16_t rd16(const uint8_t *p) { return uint16_t(p[0] | (p[1] << 8)); }
inline uint32_t rd32(const uint8_t *p)
{
    return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24);
}

constexpr size_t kMaxEntrySize = 200u * 1024u * 1024u;

// Liste les entrées via le répertoire central.
inline bool listEntries(const std::string &z, std::vector<Entry> &out, std::string &err)
{
    const uint8_t *d = reinterpret_cast<const uint8_t *>(z.data());
    const int64_t n = int64_t(z.size());
    if (n < 22) {
        err = "fichier trop petit pour être une archive zip";
        return false;
    }
    int64_t eocd = -1;
    const int64_t lowest = std::max<int64_t>(0, n - 22 - 65535);
    for (int64_t i = n - 22; i >= lowest; --i)
        if (rd32(d + i) == 0x06054b50u) {
            eocd = i;
            break;
        }
    if (eocd < 0) {
        err = "ce fichier n'est pas une archive zip valide";
        return false;
    }
    const uint16_t total = rd16(d + eocd + 10);
    const uint32_t cdOff = rd32(d + eocd + 16);
    if (total == 0xFFFF || cdOff == 0xFFFFFFFFu) {
        err = "les archives zip64 ne sont pas gérées";
        return false;
    }
    int64_t p = cdOff;
    for (int i = 0; i < total; ++i) {
        if (p + 46 > n || rd32(d + p) != 0x02014b50u) {
            err = "répertoire central de l'archive corrompu";
            return false;
        }
        Entry e;
        e.flags = rd16(d + p + 8);
        e.method = rd16(d + p + 10);
        e.crc = rd32(d + p + 16);
        e.csize = rd32(d + p + 20);
        e.usize = rd32(d + p + 24);
        const int nl = rd16(d + p + 28), xl = rd16(d + p + 30), cl = rd16(d + p + 32);
        e.offset = rd32(d + p + 42);
        if (p + 46 + nl > n) {
            err = "répertoire central de l'archive corrompu";
            return false;
        }
        e.name.assign(z.data() + p + 46, size_t(nl));
        out.push_back(std::move(e));
        p += 46 + nl + xl + cl;
    }
    return true;
}

// Extrait le contenu d'une entrée et vérifie sa taille et son CRC.
inline bool readEntry(const std::string &z, const Entry &e, std::string &out, std::string &err)
{
    const uint8_t *d = reinterpret_cast<const uint8_t *>(z.data());
    const int64_t n = int64_t(z.size());
    if (e.flags & 1) {
        err = "fichier chiffré dans l'archive : " + e.name;
        return false;
    }
    if (e.usize > kMaxEntrySize) {
        err = "fichier trop volumineux dans l'archive : " + e.name;
        return false;
    }
    const int64_t lh = e.offset;
    if (lh + 30 > n || rd32(d + lh) != 0x04034b50u) {
        err = "en-tête de fichier corrompu : " + e.name;
        return false;
    }
    const int64_t start = lh + 30 + rd16(d + lh + 26) + rd16(d + lh + 28);
    if (start + int64_t(e.csize) > n) {
        err = "données tronquées : " + e.name;
        return false;
    }
    if (e.method == 0) {
        out.assign(z.data() + start, e.csize);
    } else if (e.method == 8) {
        std::vector<uint8_t> buf;
        buf.reserve(e.usize);
        if (!lcinflate::inflateRaw(d + start, e.csize, buf, e.usize)) {
            err = "données compressées invalides : " + e.name;
            return false;
        }
        out.assign(reinterpret_cast<const char *>(buf.data()), buf.size());
    } else {
        err = "méthode de compression non gérée dans l'archive : " + e.name;
        return false;
    }
    if (out.size() != e.usize || crc32(out) != e.crc) {
        err = "fichier corrompu (CRC) : " + e.name;
        return false;
    }
    return true;
}

// Normalise un chemin d'archive ; refuse les chemins absolus, « .. » et les lecteurs Windows.
inline bool safePath(std::string &name)
{
    std::replace(name.begin(), name.end(), '\\', '/');
    if (name.empty() || name[0] == '/' || name.find(':') != std::string::npos) return false;
    size_t i = 0;
    while (i <= name.size()) {
        size_t j = name.find('/', i);
        if (j == std::string::npos) j = name.size();
        if (name.compare(i, j - i, "..") == 0) return false;
        i = j + 1;
    }
    return true;
}

// Ne garde que les vrais fichiers (pas de dossiers ni de déchets macOS) et valide les chemins.
inline bool filterFiles(const std::vector<Entry> &all, std::vector<Entry> &files, std::string &err)
{
    for (Entry e : all) {
        if (!safePath(e.name)) {
            err = "chemin dangereux dans l'archive : " + e.name;
            return false;
        }
        if (e.name.back() == '/') continue;
        if (e.name.rfind("__MACOSX/", 0) == 0 || e.name.find("/__MACOSX/") != std::string::npos) continue;
        const size_t slash = e.name.rfind('/');
        if (e.name.substr(slash == std::string::npos ? 0 : slash + 1) == ".DS_Store") continue;
        files.push_back(std::move(e));
    }
    return true;
}

inline bool endsWithFile(const std::string &n, const char *f)
{
    const std::string suf = f;
    if (n == suf) return true;
    return n.size() > suf.size() && n.compare(n.size() - suf.size(), suf.size(), suf) == 0 &&
           n[n.size() - suf.size() - 1] == '/';
}

// Dossiers (préfixes se terminant par « / », ou vide pour la racine) qui contiennent un mod.
inline std::vector<std::string> findModRoots(const std::vector<Entry> &files)
{
    std::vector<std::string> roots;
    for (const Entry &e : files)
        if (endsWithFile(e.name, "mod.conf")) roots.push_back(e.name.substr(0, e.name.size() - 8));
    if (roots.empty()) { // anciens mods sans mod.conf : on prend les init.lua les moins profonds
        auto depth = [](const std::string &s) { return int(std::count(s.begin(), s.end(), '/')); };
        int minDepth = INT_MAX;
        for (const Entry &e : files)
            if (endsWithFile(e.name, "init.lua")) minDepth = std::min(minDepth, depth(e.name));
        if (minDepth != INT_MAX)
            for (const Entry &e : files)
                if (endsWithFile(e.name, "init.lua") && depth(e.name) == minDepth)
                    roots.push_back(e.name.substr(0, e.name.size() - 8));
    }
    std::sort(roots.begin(), roots.end());
    roots.erase(std::unique(roots.begin(), roots.end()), roots.end());
    std::vector<std::string> out; // on garde les racines les plus externes
    for (const std::string &r : roots) {
        bool nested = false;
        for (const std::string &o : roots)
            if (o != r && r.compare(0, o.size(), o) == 0) {
                nested = true;
                break;
            }
        if (!nested) out.push_back(r);
    }
    return out;
}


// Noms de manifeste acceptés, par ordre de préférence (plugin.json = ancien format, encore lu).
inline const std::vector<std::string> &manifestNames()
{
    static const std::vector<std::string> n = {"plugin.yaml", "plugin.yml", "plugin.xml", "plugin.json"};
    return n;
}

// Dossier (préfixe « a/b/ » ou vide pour la racine) qui contient un manifeste : le moins profond gagne.
// `manifest` reçoit le nom du fichier trouvé. Renvoie false si l'archive ne contient aucun manifeste.
inline bool findPluginRoot(const std::vector<Entry> &files, std::string &prefix, std::string *manifest = nullptr)
{
    int best = INT_MAX, bestRank = INT_MAX;
    for (const Entry &e : files) {
        const auto &names = manifestNames();
        for (size_t r = 0; r < names.size(); ++r) {
            if (!endsWithFile(e.name, names[r].c_str())) continue;
            const int d = int(std::count(e.name.begin(), e.name.end(), '/'));
            const std::string pre = e.name.substr(0, e.name.size() - names[r].size());
            if (d < best || (d == best && int(r) < bestRank)) {
                best = d;
                bestRank = int(r);
                prefix = pre;
                if (manifest) *manifest = names[r];
            }
        }
    }
    return best != INT_MAX;
}

inline std::string trim(const std::string &s)
{
    size_t a = 0, b = s.size();
    while (a < b && (s[a] == ' ' || s[a] == '\t' || s[a] == '\r' || s[a] == '\n')) ++a;
    while (b > a && (s[b - 1] == ' ' || s[b - 1] == '\t' || s[b - 1] == '\r' || s[b - 1] == '\n')) --b;
    return s.substr(a, b - a);
}

inline std::string sanitizeName(const std::string &s)
{
    std::string r;
    for (char c : trim(s)) {
        if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
        r += ((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_') ? c : '_';
    }
    return r;
}

struct ModMeta {
    std::string name, title;
    std::vector<std::string> depends; // dépendances obligatoires (sans « ? »)
};

// mod.conf (clé = valeur) et, à défaut, depends.txt (un nom par ligne).
inline ModMeta parseMeta(const std::string &modConf, const std::string &dependsTxt)
{
    ModMeta m;
    bool hasDepends = false;
    size_t i = 0;
    while (i <= modConf.size()) {
        size_t j = modConf.find('\n', i);
        if (j == std::string::npos) j = modConf.size();
        const std::string line = trim(modConf.substr(i, j - i));
        i = j + 1;
        const size_t eq = line.find('=');
        if (line.empty() || line[0] == '#' || eq == std::string::npos) continue;
        const std::string key = trim(line.substr(0, eq)), val = trim(line.substr(eq + 1));
        if (key == "name") m.name = val;
        else if (key == "title") m.title = val;
        else if (key == "depends") {
            hasDepends = true;
            size_t a = 0;
            while (a <= val.size()) {
                size_t b = val.find(',', a);
                if (b == std::string::npos) b = val.size();
                const std::string d = trim(val.substr(a, b - a));
                if (!d.empty() && d.back() != '?') m.depends.push_back(d);
                a = b + 1;
            }
        }
    }
    if (!hasDepends) {
        size_t a = 0;
        while (a <= dependsTxt.size()) {
            size_t b = dependsTxt.find('\n', a);
            if (b == std::string::npos) b = dependsTxt.size();
            const std::string d = trim(dependsTxt.substr(a, b - a));
            if (!d.empty() && d[0] != '#' && d.back() != '?') m.depends.push_back(d);
            a = b + 1;
        }
    }
    return m;
}

}  // namespace lczip

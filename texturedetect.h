#pragma once
// Détection automatique « mod:bloc » à partir de noms de fichiers de textures.
//   default_dirt.png          -> default:dirt
//   default_grass_side.png    -> default:grass   (face « side » du bloc default:grass)
//   wool_blue.png + wool_red.png + farming_wheat.png -> wool:blue, wool:red, farming:wheat
// Avec un seul fichier, le mod est deviné d'après le premier « mot » (fiable pour les mods connus) ;
// avec plusieurs fichiers, ce qui se répète donne le nom du mod (y compris « mcl_core_* » : mod à deux mots).
// Ce fichier n'utilise que la bibliothèque standard (testable sans Qt).

#include <algorithm>
#include <cctype>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace lctex {

enum class Confidence { Sure, Probable, Unsure, NoMod };

struct Detected {
    std::string file;      // nom du fichier tel que donné (avec extension)
    std::string mod;       // « default »  (vide si aucun préfixe trouvé)
    std::string name;      // « dirt »
    std::string variant;   // « side », « top »... ("" = texture principale du bloc)
    Confidence conf = Confidence::Unsure;
    std::string ref() const { return mod.empty() ? name : mod + ":" + name; }
};

inline const std::set<std::string> &knownMods()
{
    static const std::set<std::string> s = {
        "default", "wool", "farming", "flowers", "stairs", "doors", "beds", "bones", "bucket", "carts", "fire", "tnt",
        "vessels", "xpanes", "walls", "fences", "boats", "dye", "screwdriver", "sethome", "binoculars", "creative",
        "give_initial_stuff", "map", "player_api", "sfinv", "spawn", "weather", "mtg", "basic_materials", "ethereal",
        "mobs", "moreores", "moreblocks", "technic", "pipeworks", "mesecons", "unifieddyes", "homedecor", "ncrafts",
        "nether", "caverealms", "baked_clay", "bakedclay", "lucky_block", "3d_armor", "armor", "ctf",
        "dfcaverns"};
    return s;
}

// Suffixes qui désignent une face / un état du même bloc : ils ne font pas partie de son nom.
inline const std::set<std::string> &variantWords()
{
    static const std::set<std::string> s = {"top", "bottom", "side", "sides", "front", "back", "left", "right",
                                            "inv", "inventory", "overlay", "animated", "active", "inactive", "front_active",
                                            "bot", "north", "south", "east", "west", "up", "down"};
    return s;
}

inline std::string lower(std::string s)
{
    for (char &c : s) c = (char)std::tolower((unsigned char)c);
    return s;
}

// nom de fichier sans dossier ni extension d'image
inline std::string stem(const std::string &path)
{
    std::string f = path;
    const size_t sl = f.find_last_of("/\\");
    if (sl != std::string::npos) f = f.substr(sl + 1);
    const size_t dot = f.find_last_of('.');
    if (dot != std::string::npos && dot > 0) {
        const std::string ext = lower(f.substr(dot + 1));
        if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp" || ext == "tga" || ext == "webp") f = f.substr(0, dot);
    }
    return f;
}

inline std::vector<std::string> split(const std::string &s, char sep)
{
    std::vector<std::string> out;
    std::string cur;
    for (char c : s) {
        if (c == sep) {
            if (!cur.empty()) out.push_back(cur);
            cur.clear();
        } else cur += c;
    }
    if (!cur.empty()) out.push_back(cur);
    return out;
}

inline std::string join(const std::vector<std::string> &v, size_t from, size_t to, const char *sep = "_")
{
    std::string r;
    for (size_t i = from; i < to && i < v.size(); ++i) {
        if (!r.empty()) r += sep;
        r += v[i];
    }
    return r;
}

// caractères autorisés dans un nom de mod / de bloc Luanti : a-z 0-9 _
inline std::string sanitize(const std::string &s)
{
    std::string r;
    for (char c : lower(s)) r += (std::isalnum((unsigned char)c) || c == '_') ? c : '_';
    return r;
}

// Analyse un lot de noms de fichiers. Le résultat est dans le même ordre que l'entrée.
inline std::vector<Detected> detect(const std::vector<std::string> &files)
{
    struct Item {
        std::vector<std::string> tok;
        std::string variant;
        bool explicitRef = false;
        std::string mod, name;
    };
    std::vector<Item> items(files.size());
    std::vector<Detected> out(files.size());

    for (size_t i = 0; i < files.size(); ++i) {
        out[i].file = files[i];
        std::string s = stem(files[i]);
        Item &it = items[i];
        // « default:dirt.png » : référence explicite
        const size_t colon = s.find(':');
        if (colon != std::string::npos && colon > 0 && colon + 1 < s.size()) {
            it.explicitRef = true;
            it.mod = sanitize(s.substr(0, colon));
            it.name = sanitize(s.substr(colon + 1));
            continue;
        }
        it.tok = split(sanitize(s), '_');
        // faces / états en fin de nom : default_grass_side -> bloc « default_grass », variante « side »
        while (it.tok.size() > 2 && variantWords().count(it.tok.back())) {
            it.variant = it.variant.empty() ? it.tok.back() : it.tok.back() + "_" + it.variant;
            it.tok.pop_back();
        }
        // « mod_nom_face.png » : une face seule après un unique mot de mod (ex. « default_top ») n'est pas retirée (size>2)
    }

    // Groupes par premier mot
    std::map<std::string, std::vector<size_t>> groups;
    for (size_t i = 0; i < items.size(); ++i)
        if (!items[i].explicitRef && items[i].tok.size() >= 2) groups[items[i].tok[0]].push_back(i);

    for (auto &g : groups) {
        const std::vector<size_t> &idx = g.second;
        size_t k = 1; // nombre de mots qui forment le nom du mod
        // Un mod connu a un nom d'un seul mot. Sinon, si ≥ 4 fichiers partagent le même 2e mot et gardent
        // quelque chose après, c'est que le nom du mod a deux mots (mcl_core_stone, mcl_core_dirt, ...).
        if (!knownMods().count(g.first)) {
            while (idx.size() >= 4) {
                const std::string t = items[idx[0]].tok.size() > k ? items[idx[0]].tok[k] : "";
                bool all = !t.empty();
                for (size_t i : idx)
                    if (items[i].tok.size() <= k + 1 || items[i].tok[k] != t) { all = false; break; }
                if (!all) break;
                ++k;
            }
        }
        // Les mods connus ou répétés ≥ 3 fois sont « sûrs », 2 fois « probables », seuls et inconnus « incertains ».
        Confidence c = Confidence::Unsure;
        if (knownMods().count(g.first) || idx.size() >= 3) c = Confidence::Sure;
        else if (idx.size() == 2) c = Confidence::Probable;
        for (size_t i : idx) {
            Item &it = items[i];
            it.mod = join(it.tok, 0, k);
            it.name = join(it.tok, k, it.tok.size());
            out[i].conf = c;
        }
    }

    for (size_t i = 0; i < items.size(); ++i) {
        Item &it = items[i];
        if (it.explicitRef) {
            out[i].conf = Confidence::Sure;
        } else if (it.tok.size() < 2) {
            // un seul mot : pas de préfixe de mod
            it.name = it.tok.empty() ? std::string("texture") : it.tok[0];
            out[i].conf = Confidence::NoMod;
        }
        out[i].mod = it.mod;
        out[i].name = it.name.empty() ? std::string("texture") : it.name;
        out[i].variant = it.variant;
    }
    return out;
}

// Mod le plus fréquent d'un lot (« » s'il n'y en a pas) : utile pour le résumé et pour les fichiers sans préfixe.
inline std::string dominantMod(const std::vector<Detected> &d)
{
    std::map<std::string, int> n;
    for (const Detected &x : d)
        if (!x.mod.empty()) ++n[x.mod];
    std::string best;
    int bn = 0;
    for (auto &p : n)
        if (p.second > bn) { bn = p.second; best = p.first; }
    return best;
}

// Ordre de préférence pour la texture d'un bloc qui en a plusieurs : la face (front, puis side) d'abord,
// puis la texture sans suffixe, puis le dessus, puis le reste. Plus petit = préféré.
inline int variantRank(const std::string &variant)
{
    if (variant == "front") return 0;
    if (variant == "side" || variant == "sides") return 1;
    if (variant.empty()) return 2;
    if (variant == "left" || variant == "right" || variant == "back" || variant == "north" || variant == "south" ||
        variant == "east" || variant == "west")
        return 3;
    if (variant == "top" || variant == "up") return 4;
    if (variant == "bottom" || variant == "bot" || variant == "down") return 5;
    return 6; // active, overlay, inv, animated, front_active...
}

// Dans « tiles = {...} » d'un bloc Luanti l'ordre est : dessus, dessous, droite, gauche, arrière, face.
// Texture « de face » : la 6e si elle existe, sinon la 3e (côté), sinon la 1re.
inline size_t faceTileIndex(size_t count)
{
    if (count >= 6) return 5;
    if (count >= 3) return 2;
    return 0;
}

// « stone_brick » -> « Stone brick »
inline std::string prettyLabel(const std::string &name)
{
    std::string r = name;
    for (char &c : r)
        if (c == '_') c = ' ';
    if (!r.empty()) r[0] = (char)std::toupper((unsigned char)r[0]);
    return r;
}


// ---------------------------------------------------------------------------------------------
// Lecture sommaire d'un fichier Lua de mod : cherche les « register_node("mod:nom", { ... tiles = {"x.png"} ... }) »
// et renvoie, pour chaque bloc, sa référence et sa première texture. Ce n'est pas un interpréteur Lua : seuls les
// noms et textures écrits en toutes lettres (chaînes littérales) sont reconnus.
struct LuaNode {
    std::string ref;                 // « default:dirt »
    std::string texture;             // texture de face (voir faceTileIndex)
    std::vector<std::string> tiles;  // toutes les textures de « tiles », dans l'ordre ; chacune est « base.png » ou
                                     // « base.png^par_dessus.png » (le côté de default:dirt_with_grass = terre + herbe)
};

// lit une chaîne littérale « ... » ou '...' à partir de pos (qui pointe sur le guillemet) ; pos passe après le guillemet fermant
inline bool readLiteral(const std::string &s, size_t &pos, std::string &out)
{
    if (pos >= s.size() || (s[pos] != '"' && s[pos] != '\'')) return false;
    const char q = s[pos];
    size_t i = pos + 1;
    std::string r;
    while (i < s.size() && s[i] != q && s[i] != '\n') {
        if (s[i] == '\\' && i + 1 < s.size()) ++i;
        r += s[i++];
    }
    if (i >= s.size() || s[i] != q) return false;
    out = r;
    pos = i + 1;
    return true;
}

// « default_dirt.png^default_grass_side.png^[colorize:red » -> « default_dirt.png^default_grass_side.png » :
// on garde la base et les textures posées par-dessus (« ^ »), les modificateurs « [... » sont ignorés.
// Renvoie "" si l'expression ne commence pas par un fichier .png lisible.
inline std::string pngSpec(const std::string &lit)
{
    const std::vector<std::string> parts = split(lit, '^');
    std::string out;
    for (size_t i = 0; i < parts.size(); ++i) {
        std::string p = parts[i];
        size_t a = 0, b = p.size();
        while (a < b && p[a] == '(') ++a;
        while (b > a && p[b - 1] == ')') --b;
        p = p.substr(a, b - a);
        const std::string low = lower(p);
        const bool png = p.size() > 4 && p[0] != '[' && low.compare(low.size() - 4, 4, ".png") == 0;
        if (!png) {
            if (i == 0) return std::string();
            continue;
        }
        if (!out.empty()) out += "^";
        out += p;
    }
    return out;
}

// fichiers d'une expression de texture : « a.png^b.png » -> {a.png, b.png}
inline std::vector<std::string> specFiles(const std::string &spec) { return split(spec, '^'); }

inline std::vector<LuaNode> scanNodes(const std::string &lua)
{
    std::vector<LuaNode> out;
    const std::string key = "register_node";
    size_t at = 0;
    while ((at = lua.find(key, at)) != std::string::npos) {
        size_t p = at + key.size();
        at = p;
        while (p < lua.size() && (lua[p] == ' ' || lua[p] == '\t')) ++p;
        if (p >= lua.size() || lua[p] != '(') continue;
        ++p;
        while (p < lua.size() && (lua[p] == ' ' || lua[p] == '\t' || lua[p] == '\n' || lua[p] == '\r')) ++p;
        std::string name;
        if (!readLiteral(lua, p, name)) continue;
        if (!name.empty() && name[0] == ':') name.erase(0, 1);
        if (name.find(':') == std::string::npos || name.back() == ':') continue;
        // le corps de la définition s'arrête au prochain « register_ »
        size_t end = lua.find("register_", p);
        if (end == std::string::npos || end > p + 6000) end = std::min(lua.size(), p + 6000);
        LuaNode n;
        n.ref = name;
        size_t t = p;
        while ((t = lua.find("tiles", t)) != std::string::npos && t < end) {
            size_t q = t + 5;
            while (q < end && (lua[q] == ' ' || lua[q] == '\t')) ++q;
            if (q >= end || lua[q] != '=') { t = q; continue; }
            ++q;
            int depth = 0;
            bool opened = false;
            while (q < end && q < t + 800) {
                const char c = lua[q];
                if (c == '{') { ++depth; opened = true; ++q; }
                else if (c == '}') { --depth; ++q; if (opened && depth <= 0) break; }
                else if (c == '"' || c == '\'') {
                    const size_t startLit = q;
                    std::string lit;
                    if (!readLiteral(lua, q, lit)) break;
                    // « mod.."_x.png" » ou « "x" .. suffixe » : texture construite, non lisible
                    size_t a = startLit;
                    while (a > 0 && (lua[a - 1] == ' ' || lua[a - 1] == '\t')) --a;
                    size_t b = q;
                    while (b < lua.size() && (lua[b] == ' ' || lua[b] == '\t')) ++b;
                    const bool concat = (a >= 2 && lua[a - 1] == '.' && lua[a - 2] == '.') ||
                                        (b + 1 < lua.size() && lua[b] == '.' && lua[b + 1] == '.');
                    const std::string spec = pngSpec(lit);
                    if (!concat && !spec.empty()) {
                        n.tiles.push_back(spec);
                        if (n.tiles.size() >= 6) break;
                    }
                } else if (!opened && (c == ',' || c == '\n')) break; // « tiles = variable » : rien à lire
                else ++q;
            }
            break;
        }
        if (!n.tiles.empty()) n.texture = n.tiles[faceTileIndex(n.tiles.size())];
        out.push_back(n);
    }
    return out;
}

} // namespace lctex

#pragma once
// Import d'un mod Luanti existant (dossier ou archive .zip) comme PROJET LuantiCreator.
//
// LuantiCreator n'est pas un interpréteur Lua : il lit les définitions écrites en toutes lettres
// (register_node / register_craftitem / register_tool) et en tire les blocs, objets, nourritures et outils avec leurs
// textures (copiées dans ~/.local/share/LuantiCreator/imports/<id>/textures). Le reste du code du mod
// (callbacks, entités, commandes...) n'est pas converti : le résumé de l'import le rappelle.

#include <QtWidgets>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <cctype>
#include <string>
#include <vector>

#include "appcore.h"
#include "fsutil.h"
#include "generator.h"
#include "texturedetect.h"
#include "zipread.h"

namespace lcimport {

// identifiant de projet libre : « mon_mod », « mon_mod_2 »...
inline QString uniqueProjectId(const QString &wanted)
{
    QString base = identOf(wanted);
    if (base.isEmpty()) base = "mon_mod";
    QString id = base;
    for (int k = 2; QFile::exists(lcProjectFile(id)); ++k) id = base + "_" + QString::number(k);
    return id;
}

struct Def {
    QString ref;       // « mod:nom »
    size_t from = 0;   // corps de la définition : [from, to[
    size_t to = 0;
};

// « register_xxx("mod:nom", { ... » -> références et zones de texte à analyser
inline std::vector<Def> scanDefs(const std::string &lua, const char *key)
{
    std::vector<Def> out;
    const std::string k = key;
    size_t at = 0;
    while ((at = lua.find(k, at)) != std::string::npos) {
        size_t p = at + k.size();
        at = p;
        while (p < lua.size() && (lua[p] == ' ' || lua[p] == '\t')) ++p;
        if (p >= lua.size() || lua[p] != '(') continue;
        ++p;
        while (p < lua.size() && (lua[p] == ' ' || lua[p] == '\t' || lua[p] == '\n' || lua[p] == '\r')) ++p;
        std::string name;
        if (!lctex::readLiteral(lua, p, name)) continue;
        if (!name.empty() && name[0] == ':') name.erase(0, 1);
        if (name.find(':') == std::string::npos || name.back() == ':') continue;
        size_t end = lua.find("register_", p);
        if (end == std::string::npos || end > p + 6000) end = std::min(lua.size(), p + 6000);
        Def d;
        d.ref = QString::fromStdString(name);
        d.from = p;
        d.to = end;
        out.push_back(d);
    }
    return out;
}

inline bool isWordChar(char c) { return std::isalnum(static_cast<unsigned char>(c)) || c == '_'; }

// valeur « cle = "texte" » (ou cle = S("texte")) dans [from, to[ ; chaîne vide si absente
inline QString literalOf(const std::string &lua, size_t from, size_t to, const char *key)
{
    const std::string k = key;
    size_t p = from;
    while ((p = lua.find(k, p)) != std::string::npos && p < to) {
        const bool wordStart = p == 0 || !isWordChar(lua[p - 1]);
        size_t q = p + k.size();
        p = q;
        if (!wordStart) continue;
        while (q < to && (lua[q] == ' ' || lua[q] == '\t')) ++q;
        if (q >= to || lua[q] != '=') continue;
        ++q;
        while (q < to && (lua[q] == ' ' || lua[q] == '\t' || lua[q] == '\n' || lua[q] == '\r')) ++q;
        size_t r = q; // traduction : S("texte"), core.gettext("texte")
        while (r < to && (isWordChar(lua[r]) || lua[r] == '.')) ++r;
        if (r > q && r < to && lua[r] == '(') {
            q = r + 1;
            while (q < to && (lua[q] == ' ' || lua[q] == '\t')) ++q;
        }
        std::string lit;
        if (q < to && lctex::readLiteral(lua, q, lit)) return QString::fromUtf8(lit.data(), int(lit.size()));
    }
    return QString();
}

// valeur « cle = 12 » dans [from, to[ ; def si absente
inline int numberOf(const std::string &lua, size_t from, size_t to, const char *key, int def)
{
    const std::string k = key;
    size_t p = from;
    while ((p = lua.find(k, p)) != std::string::npos && p < to) {
        const bool wordStart = p == 0 || !isWordChar(lua[p - 1]);
        size_t q = p + k.size();
        p = q;
        if (!wordStart) continue;
        while (q < to && (lua[q] == ' ' || lua[q] == '\t')) ++q;
        if (q >= to || lua[q] != '=') continue;
        ++q;
        while (q < to && (lua[q] == ' ' || lua[q] == '\t')) ++q;
        size_t e = q;
        if (e < to && lua[e] == '-') ++e;
        while (e < to && std::isdigit(static_cast<unsigned char>(lua[e]))) ++e;
        if (e > q) return QString::fromStdString(lua.substr(q, e - q)).toInt();
    }
    return def;
}

// Copie les textures d'un mod dans le dossier du projet ; renvoie les chemins absolus des copies.
struct TexCopier {
    QString srcDir, dstDir;
    QMap<QString, QString> done;
    int copied = 0, missing = 0;

    QString copy(const QString &file)
    {
        const auto it = done.constFind(file);
        if (it != done.constEnd()) return it.value();
        QString result;
        const QString src = srcDir + "/" + file;
        if (!file.isEmpty() && !file.contains('/') && !file.contains("..") && QFile::exists(src)) {
            QDir().mkpath(dstDir);
            const QString dst = dstDir + "/" + QFileInfo(file).completeBaseName() + ".png";
            bool ok;
            if (file.endsWith(".png", Qt::CaseInsensitive)) {
                QFile::remove(dst);
                ok = QFile::copy(src, dst);
            } else {
                const QImage img(src);
                ok = !img.isNull() && img.save(dst, "PNG");
            }
            if (ok) {
                result = dst;
                ++copied;
            }
        }
        if (result.isEmpty()) ++missing;
        done.insert(file, result);
        return result;
    }

    // « base.png^dessus.png^dessus2.png » -> texture, overlay, overlay2
    void fill(QJsonObject &o, const std::string &spec)
    {
        if (spec.empty()) return;
        const std::vector<std::string> parts = lctex::specFiles(spec);
        static const char *keys[] = {"texture", "overlay", "overlay2"};
        for (size_t i = 0; i < parts.size() && i < 3; ++i) {
            const QString f = copy(QString::fromStdString(parts[i]));
            if (!f.isEmpty()) o[keys[i]] = f;
            else if (i == 0) return; // sans base, les textures par-dessus n'ont pas de sens
        }
    }
};

struct Result {
    bool ok = false;
    QString projectPath, id, summary, error;
};

// Convertit le dossier d'un mod (déjà décompressé) en projet. `modName` : identifiant du mod (mod.conf / dossier).
inline Result importModDir(const QString &dir, const QString &modName, const QString &title, const QStringList &deps,
                           const QString &description)
{
    Result res;
    const QString id = uniqueProjectId(modName);
    TexCopier tc;
    tc.srcDir = dir + "/textures";
    tc.dstDir = lcImportsDir() + "/" + id + "/textures";
    QDir(lcImportsDir() + "/" + id).removeRecursively();

    QJsonArray nodes, items, foods, tools;
    int foreign = 0, luaFiles = 0;
    QSet<QString> seen;
    const QString mod = identOf(modName);

    QDirIterator it(dir, QStringList() << "*.lua", QDir::Files, QDirIterator::Subdirectories);
    QStringList luaPaths;
    while (it.hasNext()) luaPaths << it.next();
    luaPaths.sort();
    for (const QString &lp : luaPaths) {
        QFile f(lp);
        if (f.size() > 2 * 1024 * 1024 || !f.open(QIODevice::ReadOnly)) continue;
        const QByteArray raw = f.readAll();
        f.close();
        const std::string lua(raw.constData(), size_t(raw.size()));
        ++luaFiles;

        // le nom affiché et le reste d'un objet : description, image d'inventaire, pile max
        auto nameOf = [&](const QString &ref) { return identOf(ref.section(':', 1)); };
        auto describe = [&](const Def &d, const QString &ref) {
            QString desc = literalOf(lua, d.from, d.to, "description").section('\n', 0, 0).trimmed();
            if (desc.isEmpty()) desc = QString::fromStdString(lctex::prettyLabel(ref.section(':', 1).toStdString()));
            return desc;
        };

        // --- blocs
        const std::vector<lctex::LuaNode> tiles = lctex::scanNodes(lua);
        for (const Def &d : scanDefs(lua, "register_node")) {
            if (identOf(d.ref.section(':', 0, 0)) != mod) { ++foreign; continue; }
            if (seen.contains("n:" + d.ref)) continue;
            seen.insert("n:" + d.ref);
            QJsonObject o;
            o["name"] = nameOf(d.ref);
            o["description"] = describe(d, d.ref);
            for (const lctex::LuaNode &n : tiles)
                if (QString::fromStdString(n.ref) == d.ref) {
                    tc.fill(o, n.texture);
                    break;
                }
            const QString dt = literalOf(lua, d.from, d.to, "drawtype");
            if (QStringList{"normal", "glasslike", "allfaces", "plantlike", "torchlike", "airlike"}.contains(dt)) o["drawtype"] = dt;
            const int light = numberOf(lua, d.from, d.to, "light_source", 0);
            if (light > 0 && light <= 14) o["light"] = light;
            nodes.append(o);
        }

        // --- objets et nourritures
        for (const Def &d : scanDefs(lua, "register_craftitem")) {
            if (identOf(d.ref.section(':', 0, 0)) != mod) { ++foreign; continue; }
            if (seen.contains("i:" + d.ref)) continue;
            seen.insert("i:" + d.ref);
            QJsonObject o;
            o["name"] = nameOf(d.ref);
            o["description"] = describe(d, d.ref);
            tc.fill(o, lctex::pngSpec(literalOf(lua, d.from, d.to, "inventory_image").toStdString()));
            const int stack = numberOf(lua, d.from, d.to, "stack_max", 99);
            if (stack > 0 && stack <= 999) o["stack"] = stack;
            const size_t eat = lua.find("item_eat(", d.from);
            if (eat != std::string::npos && eat < d.to) { // nourriture : points de vie rendus
                size_t q = eat + 9;
                while (q < d.to && lua[q] == ' ') ++q;
                size_t e = q;
                while (e < d.to && std::isdigit(static_cast<unsigned char>(lua[e]))) ++e;
                o["hp"] = e > q ? QString::fromStdString(lua.substr(q, e - q)).toInt() : 2;
                foods.append(o);
            } else {
                items.append(o);
            }
        }

        // --- outils
        for (const Def &d : scanDefs(lua, "register_tool")) {
            if (identOf(d.ref.section(':', 0, 0)) != mod) { ++foreign; continue; }
            if (seen.contains("t:" + d.ref)) continue;
            seen.insert("t:" + d.ref);
            QJsonObject o;
            o["name"] = nameOf(d.ref);
            o["description"] = describe(d, d.ref);
            tc.fill(o, lctex::pngSpec(literalOf(lua, d.from, d.to, "inventory_image").toStdString()));
            tools.append(o);
        }
    }

    QJsonObject proj;
    proj["name"] = id;
    proj["title"] = title.isEmpty() ? QString::fromStdString(lctex::prettyLabel(modName.toStdString())) : title;
    proj["desc"] = description;
    proj["deps"] = deps.isEmpty() ? QString("default") : deps.join(", ");
    proj["nodes"] = nodes;
    proj["items"] = items;
    proj["foods"] = foods;
    proj["tools"] = tools;

    QDir().mkpath(lcProjectsDir());
    QFile pf(lcProjectFile(id));
    if (!pf.open(QIODevice::WriteOnly)) {
        res.error = QObject::tr("Impossible d'écrire %1").arg(lcProjectFile(id));
        return res;
    }
    pf.write(QJsonDocument(proj).toJson());
    pf.close();

    res.ok = true;
    res.id = id;
    res.projectPath = lcProjectFile(id);
    QString s = QObject::tr("Projet « %1 » créé : %2 bloc(s), %3 objet(s), %4 nourriture(s), %5 outil(s) (%6 fichier(s) Lua lus).")
                    .arg(id).arg(nodes.size()).arg(items.size()).arg(foods.size()).arg(tools.size()).arg(luaFiles);
    s += "\n" + QObject::tr("%1 texture(s) copiée(s)").arg(tc.copied);
    if (tc.missing) s += QObject::tr(", %1 introuvable(s) ou illisible(s)").arg(tc.missing);
    s += ".";
    if (foreign) s += "\n" + QObject::tr("%1 définition(s) d'autres mods ignorée(s).").arg(foreign);
    if (id != identOf(modName)) s += "\n" + QObject::tr("L'identifiant « %1 » existait déjà : le projet s'appelle « %2 ».").arg(identOf(modName), id);
    s += "\n\n" + QObject::tr("Seuls les blocs, objets, nourritures et outils déclarés en toutes lettres sont importés. "
                              "Le reste du code du mod (entités, commandes, callbacks, générations...) n'est pas converti : "
                              "refaites-le avec les pages de LuantiCreator si besoin.");
    res.summary = s;
    return res;
}

// description = ... de mod.conf (ModMeta ne la contient pas)
inline QString confValue(const QString &dir, const char *key)
{
    const std::string conf = readTextFile(dir + "/mod.conf");
    size_t i = 0;
    while (i <= conf.size()) {
        size_t j = conf.find('\n', i);
        if (j == std::string::npos) j = conf.size();
        const std::string line = lczip::trim(conf.substr(i, j - i));
        i = j + 1;
        const size_t eq = line.find('=');
        if (line.empty() || line[0] == '#' || eq == std::string::npos) continue;
        if (lczip::trim(line.substr(0, eq)) == key) return QString::fromStdString(lczip::trim(line.substr(eq + 1)));
    }
    return QString();
}

// Importe un dossier de mod déjà décompressé.
inline Result importFolder(const QString &dir)
{
    ZipModInfo info;
    QString err;
    Result r;
    if (!inspectModDir(dir, info, &err)) {
        r.error = err;
        return r;
    }
    return importModDir(dir, info.name, info.title, info.depends, confValue(dir, "description"));
}

// Importe une archive .zip (un mod ou un modpack : si plusieurs mods, on demande lequel).
inline Result importZip(QWidget *parent, const QString &zipPath)
{
    Result r;
    QVector<ZipModInfo> mods;
    QString err;
    if (!zipListMods(zipPath, mods, &err)) {
        r.error = err;
        return r;
    }
    ZipModInfo pick = mods[0];
    if (mods.size() > 1) {
        QStringList names;
        for (const ZipModInfo &m : mods) names << m.name;
        bool ok = false;
        const QString c = QInputDialog::getItem(parent, QObject::tr("Modpack"),
                                                QObject::tr("Cette archive contient plusieurs mods. Lequel importer ?"), names, 0,
                                                false, &ok);
        if (!ok) {
            r.error = QString(); // annulé
            return r;
        }
        pick = mods[names.indexOf(c)];
    }
    // extraction dans un dossier temporaire (rien n'est exécuté), puis lecture comme un dossier
    const QString tmp = lcImportsDir() + "/_tmp";
    QDir(tmp).removeRecursively();
    QDir().mkpath(tmp);
    QVector<ZipModInfo> one;
    one << pick;
    if (!zipExtractMods(zipPath, one, tmp, &err)) {
        QDir(tmp).removeRecursively();
        r.error = err;
        return r;
    }
    const QString modDir = tmp + "/" + pick.name;
    r = importModDir(modDir, pick.name, pick.title, pick.depends, confValue(modDir, "description"));
    QDir(tmp).removeRecursively();
    return r;
}

} // namespace lcimport

#pragma once
// Plugins de LuantiCreator (archives .zip) : icônes, textures et blocs pour les sélecteurs, modèles
// prédéfinis, extraits de code Lua et nouvelles fonctionnalités (pages d'édition + gabarits Lua).
//
// Un plugin est purement déclaratif (plugin.yaml ou plugin.xml + images + fichiers texte). Aucun code natif n'est
// exécuté : ses extraits Lua ne servent qu'à être ajoutés au mod généré, lu par Luanti.
//
//   ~/.local/share/LuantiCreator/plugins/<id>/plugin.yaml   (ou plugin.xml ; plugin.json = ancien format, encore lu)
//
// Voir PLUGINS.md pour le format complet.

#include <QtWidgets>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <string>
#include <vector>

#include "editor.h"
#include "fsutil.h"
#include "generator.h" // identOf
#include "markup.h"
#include "scriptrun.h"
#include "template.h"
#include "uihooks.h"
#include "uikeys.h" // généré par gen_uikeys.py : clés d'icônes qui existent dans le code
#include "zipread.h"

// Une clé d'icône existe-t-elle dans l'application ? Détection dans le code : liste générée à partir des appels
// UiHooks::bind* (uikeys.h), clés déclarées à l'exécution par les fenêtres déjà ouvertes, puis familles ouvertes
// (cartes, catégories, boutons) dont les clés dépendent des pages et des plugins.
inline bool lcIconKeyExists(const QString &key)
{
    if (key == "app" || key.startsWith("card.") || key.startsWith("cat.") || key.startsWith("btn.") || key.contains(".btn.")) return true;
    if (UiHooks::seenKeys().contains(key)) return true;
    static const QSet<QString> fromCode = [] {
        QSet<QString> s;
        std::size_t n = 0;
        const char *const *k = lckeys::codeIconKeys(&n);
        for (std::size_t i = 0; i < n; ++i) s.insert(QString::fromUtf8(k[i]));
        return s;
    }();
    return fromCode.contains(key);
}

// ---------------------------------------------------------------- manifeste (YAML / XML / ancien JSON)
namespace lcman {

inline QStringList names() { return {"plugin.yaml", "plugin.yml", "plugin.xml", "plugin.json"}; }
inline bool isManifest(const QString &rel) { return names().contains(rel); }
// nom du manifeste présent dans `dir` (ordre : yaml, yml, xml, json) ou chaîne vide
inline QString find(const QString &dir)
{
    for (const QString &n : names())
        if (QFile::exists(dir + "/" + n)) return n;
    return QString();
}
inline QString ext(const QString &fileName) { return QFileInfo(fileName).suffix().toLower(); }

inline QJsonValue toJson(const lcmk::Value &v)
{
    switch (v.t) {
    case lcmk::Value::Null: return QJsonValue(QJsonValue::Null);
    case lcmk::Value::Bool: return QJsonValue(v.b);
    case lcmk::Value::Num: return QJsonValue(v.n);
    case lcmk::Value::Str: return QJsonValue(QString::fromUtf8(v.s.data(), int(v.s.size())));
    case lcmk::Value::Arr: {
        QJsonArray a;
        for (const lcmk::Value &x : v.a) a.append(toJson(x));
        return a;
    }
    case lcmk::Value::Obj: {
        QJsonObject o;
        for (const auto &kv : v.o) o.insert(QString::fromUtf8(kv.first.data(), int(kv.first.size())), toJson(kv.second));
        return o;
    }
    }
    return QJsonValue();
}

// QJsonObject trie ses clés ; on les remet dans un ordre lisible pour l'écriture
inline int keyRank(const QString &k)
{
    static const QStringList order = {"format", "id", "name", "version", "author", "description", "icons", "textures",
                                      "blocks", "blocksets", "presets", "snippets", "features",
                                      "ref", "label", "category", "texture", "names",
                                      "tab", "values",
                                      "key", "title", "type", "default", "min", "max", "options", "hint", "filter",
                                      "file", "icon", "icon_only", "prefix", "template", "fields"};
    const int i = int(order.indexOf(k));
    return i < 0 ? 1000 : i;
}

inline lcmk::Value fromJson(const QJsonValue &v)
{
    if (v.isBool()) return lcmk::Value::boolean(v.toBool());
    if (v.isDouble()) return lcmk::Value::num(v.toDouble());
    if (v.isString()) return lcmk::Value::str(v.toString().toStdString());
    if (v.isArray()) {
        lcmk::Value a = lcmk::Value::arr();
        for (const QJsonValue &x : v.toArray()) a.a.push_back(fromJson(x));
        return a;
    }
    if (v.isObject()) {
        const QJsonObject o = v.toObject();
        QStringList keys = o.keys();
        std::stable_sort(keys.begin(), keys.end(), [](const QString &a, const QString &b) { return keyRank(a) < keyRank(b); });
        lcmk::Value r = lcmk::Value::obj();
        for (const QString &k : keys) r.o.emplace_back(k.toStdString(), fromJson(o.value(k)));
        return r;
    }
    return lcmk::Value();
}

// lit un manifeste ; en cas d'erreur, `err` contient un message avec la ligne
inline bool parse(const QByteArray &data, const QString &fileName, QJsonObject &out, QString &err)
{
    if (ext(fileName) == "json") {
        QJsonParseError pe;
        const QJsonDocument d = QJsonDocument::fromJson(data, &pe);
        if (!d.isObject()) {
            err = QObject::tr("%1 invalide : %2 (position %3)").arg(fileName, pe.errorString()).arg(pe.offset);
            return false;
        }
        out = d.object();
        return true;
    }
    lcmk::Value v;
    std::string e;
    int line = 0;
    if (!lcmk::parseManifest(data.toStdString(), ext(fileName).toStdString(), v, e, &line)) {
        err = line > 0 ? QObject::tr("%1 invalide, ligne %2 : %3").arg(fileName).arg(line).arg(QString::fromStdString(e))
                       : QObject::tr("%1 invalide : %2").arg(fileName, QString::fromStdString(e));
        return false;
    }
    out = toJson(v).toObject();
    return true;
}

inline QByteArray serialize(const QJsonObject &o, const QString &fileName)
{
    if (ext(fileName) == "json") return QJsonDocument(o).toJson();
    return QByteArray::fromStdString(lcmk::writeManifest(fromJson(o), ext(fileName).toStdString()));
}

} // namespace lcman

inline QString lcDataDir()
{
    QString d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (d.isEmpty()) d = QDir::homePath() + "/.local/share/LuantiCreator";
    return d;
}

struct PluginBlock {
    QString ref, label, category, texture; // texture : chemin absolu ("" = aucune)
};

struct PluginPreset {
    QString tab, name;
    QVariantMap values;
};

struct PluginSnippet {
    QString id, name, desc, file, text;
    bool tpl = false;       // rendu avec le moteur de gabarits ({{modname}}...)
    bool onByDefault = false;
};

// Script de plugin : « event » = "generate" (à chaque génération du mod), "manual" (bouton « Exécuter »)
// ou "startup" (au lancement de LuantiCreator et à chaque rechargement des plugins)
struct PluginScript {
    QString id, name, desc, file, interpreter, event;
};

struct PluginFeature {
    bool iconOnly = false;
    QString id, key, title, desc, icon, prefix, tplFile, tplText; // key = "plugin.<plugin>.<id>"
    QString script, scriptInterpreter; // script qui génère du code à partir des éléments (facultatif, avec ou sans gabarit)
    QVector<Field> fields;
};

struct Plugin {
    QString id, name, version, author, description, dir;
    bool enabled = true;
    QMap<QString, QString> icons;    // clé d'icône -> chemin absolu
    QSet<QString> iconOnly;          // clés dont l'icône remplace le texte (« icon_only »)
    QMap<QString, QString> textures; // référence -> chemin absolu
    QVector<PluginBlock> blocks;
    QVector<PluginPreset> presets;
    QVector<PluginSnippet> snippets;
    QVector<PluginFeature> features;
    QVector<PluginScript> scripts;
    bool bundled = false; // intégré à l'exécutable (fichier .bundled)
    bool core = false;    // « core: true » : le plugin déclare vouloir modifier LuantiCreator (module Python luanti_creator_core)
    QStringList problems;

    bool hasScripts() const
    {
        if (!scripts.isEmpty()) return true;
        for (const PluginFeature &f : features)
            if (!f.script.isEmpty()) return true;
        return false;
    }
};

struct ZipPluginInfo {
    QString prefix, id, name, version, author, description;
    int blocks = 0, icons = 0, presets = 0, snippets = 0, features = 0, files = 0, skipped = 0, scripts = 0;
    bool exists = false; // un plugin de même identifiant est déjà installé
};

inline const QStringList &pluginTabKeys()
{
    static const QStringList k = {"nodes", "models", "items", "foods", "tools", "crafts",
                                  "ores",  "biomes", "decos", "entities", "cmds"};
    return k;
}

class PluginHost {
public:
    static PluginHost &I()
    {
        static PluginHost h;
        return h;
    }

    static QString root() { return lcDataDir() + "/plugins"; }

    const QVector<Plugin> &plugins() const { return list_; }

    const Plugin *find(const QString &id) const
    {
        for (const Plugin &p : list_)
            if (p.id == id) return &p;
        return nullptr;
    }

    // ------------------------------------------------------------ chargement
    void reload()
    {
        list_.clear();
        icons_.clear();
        iconOnly_.clear();
        QDir().mkpath(root());
        const QStringList disabled = QSettings("LuantiCreator", "LuantiCreator").value("pluginsDisabled").toStringList();
        const QStringList dirs = QDir(root()).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
        for (const QString &d : dirs) {
            if (d.startsWith('.')) continue;
            Plugin p;
            p.dir = root() + "/" + d;
            if (lcman::find(p.dir).isEmpty()) continue;
            loadOne(p.dir, p);
            if (p.id.isEmpty()) p.id = d;
            if (p.name.isEmpty()) p.name = p.id;
            p.enabled = !disabled.contains(p.id);
            p.bundled = QFile::exists(p.dir + "/.bundled");
            list_.append(p);
        }
    }

    void setEnabled(const QString &id, bool on)
    {
        QSettings st("LuantiCreator", "LuantiCreator");
        QStringList d = st.value("pluginsDisabled").toStringList();
        d.removeAll(id);
        if (!on) d << id;
        st.setValue("pluginsDisabled", d);
        for (Plugin &p : list_)
            if (p.id == id) p.enabled = on;
        icons_.clear();
        iconOnly_.clear();
    }

    // ------------------------------------------------------------ données fusionnées
    // Icône fournie par un plugin actif (le premier par ordre alphabétique d'identifiant).
    QIcon icon(const QString &key)
    {
        auto it = icons_.find(key);
        if (it != icons_.end()) return it.value();
        QIcon ic;
        bool only = false;
        const auto ci = coreIcons_.constFind(key); // icône imposée par un plugin autorisé à modifier LuantiCreator
        if (ci != coreIcons_.constEnd()) {
            const QPixmap px(ci.value().first);
            if (!px.isNull()) {
                icons_.insert(key, QIcon(px));
                iconOnly_.insert(key, ci.value().second);
                return icons_.value(key);
            }
        }
        for (const Plugin &p : list_) {
            if (!p.enabled || !p.icons.contains(key)) continue;
            const QPixmap px(p.icons.value(key));
            if (!px.isNull()) {
                ic = QIcon(px);
                only = p.iconOnly.contains(key);
                break;
            }
        }
        icons_.insert(key, ic);
        iconOnly_.insert(key, only);
        return ic;
    }

    // true si l'icône active de cette clé doit s'afficher sans texte
    bool iconOnly(const QString &key)
    {
        icon(key);
        return iconOnly_.value(key, false);
    }

    QIcon categoryIcon(const QString &category) { return icon("cat." + identOf(category)); }

    QString texture(const QString &ref) const
    {
        for (const Plugin &p : list_) {
            if (!p.enabled) continue;
            auto it = p.textures.find(ref);
            if (it != p.textures.end()) return it.value();
            for (const PluginBlock &b : p.blocks)
                if (b.ref == ref && !b.texture.isEmpty()) return b.texture;
        }
        return QString();
    }

    QVector<PluginBlock> blocks() const
    {
        QVector<PluginBlock> r;
        for (const Plugin &p : list_)
            if (p.enabled) r += p.blocks;
        return r;
    }

    QVector<PluginPreset> presets(const QString &tabKey) const
    {
        QVector<PluginPreset> r;
        for (const Plugin &p : list_)
            if (p.enabled)
                for (const PluginPreset &s : p.presets)
                    if (s.tab == tabKey) r.append(s);
        return r;
    }

    QVector<PluginFeature> features() const
    {
        QVector<PluginFeature> r;
        for (const Plugin &p : list_)
            if (p.enabled) r += p.features;
        return r;
    }

    // « plugin/extrait » -> extrait
    const PluginSnippet *snippet(const QString &full, const Plugin **owner = nullptr) const
    {
        const int slash = full.indexOf('/');
        if (slash < 0) return nullptr;
        const Plugin *p = find(full.left(slash));
        if (!p || !p->enabled) return nullptr;
        for (const PluginSnippet &s : p->snippets)
            if (s.id == full.mid(slash + 1)) {
                if (owner) *owner = p;
                return &s;
            }
        return nullptr;
    }

    // ------------------------------------------------------------ génération du Lua
    // Lua ajouté au mod : extraits cochés + une section par fonctionnalité de plugin.
    // root vide = aperçu (rien n'est copié).
    QString generateLua(const QStringList &snippetIds, const QMap<QString, QList<QVariantMap>> &featureData,
                        const QString &modid, const QString &root, QStringList &warns) const
    {
        QString out;
        luaPatches_.clear(); // les modifications du Lua sont redemandées par les scripts à chaque génération
        for (const QString &full : snippetIds) {
            const Plugin *p = nullptr;
            const PluginSnippet *s = snippet(full, &p);
            if (!s) continue;
            QString text = s->text;
            if (s->tpl) {
                lctpl::Vars vars;
                vars["modname"] = lctpl::Value::str(modid.toStdString());
                std::string res, err;
                std::vector<std::string> w;
                if (!lctpl::render(text.toStdString(), vars, res, err, &w)) {
                    warns << QObject::tr("Plugin %1, extrait « %2 » : %3").arg(p->name, s->name, QString::fromStdString(err));
                    continue;
                }
                for (const std::string &x : w)
                    warns << QObject::tr("Plugin %1, extrait « %2 » : %3").arg(p->name, s->name, QString::fromStdString(x));
                text = QString::fromStdString(res);
            }
            out += "-- Plugin « " + p->name + " » : " + s->name + "\n" + text;
            if (!text.endsWith('\n')) out += "\n";
            out += "\n";
        }
        for (const Plugin &p : list_) {
            if (!p.enabled) continue;
            for (const PluginFeature &f : p.features) {
                const QList<QVariantMap> items = featureData.value(f.key);
                if (items.isEmpty()) continue;
                if (!f.tplText.isEmpty() || f.script.isEmpty()) {
                    out += "-- Plugin « " + p.name + " » : " + f.title + "\n";
                    out += renderFeature(p, f, items, modid, root, warns);
                }
                if (!f.script.isEmpty()) { // le script produit du Lua à partir de tous les éléments de la fonctionnalité
                    QJsonObject c = baseContext(p, "feature");
                    QJsonObject fo;
                    fo["id"] = f.id;
                    fo["key"] = f.key;
                    fo["title"] = f.title;
                    fo["prefix"] = f.prefix;
                    c["feature"] = fo;
                    c["items"] = itemsJson(items, modid);
                    out += scriptOutput(p, f.title, f.script, f.scriptInterpreter, c, root, warns);
                }
            }
            for (const PluginScript &s : p.scripts) // scripts « generate » : une fois à chaque génération
                if (s.event == "generate") out += scriptOutput(p, s.name, s.file, s.interpreter, baseContext(p, "generate"), root, warns);
        }
        return out;
    }

    // ------------------------------------------------------------ scripts
    // Contexte commun (application, mod, projet) fourni par le programme avant la génération.
    void setContext(const QJsonObject &c) { ctx_ = c; }

    // Empreinte du plugin (tous les fichiers sauf les images) : une modification invalide l'autorisation.
    static QString fingerprint(const Plugin &p)
    {
        QStringList files;
        QDirIterator it(p.dir, QDir::Files, QDirIterator::Subdirectories);
        while (it.hasNext()) {
            const QString rel = QDir(p.dir).relativeFilePath(it.next());
            if (!rel.endsWith(".png", Qt::CaseInsensitive)) files << rel;
        }
        files.sort();
        QCryptographicHash h(QCryptographicHash::Sha256);
        for (const QString &rel : files) {
            h.addData(rel.toUtf8());
            h.addData(QByteArray(1, '\0'));
            QFile f(p.dir + "/" + rel);
            if (f.open(QIODevice::ReadOnly)) h.addData(f.read(8 * 1024 * 1024));
            h.addData(QByteArray(1, '\1'));
        }
        return QString::fromLatin1(h.result().toHex());
    }
    // Les scripts d'un plugin ne s'exécutent que si l'utilisateur les a autorisés pour CETTE version du plugin.
    bool scriptsAllowed(const Plugin &p) const
    {
        const QString saved = QSettings("LuantiCreator", "LuantiCreator").value("pluginScripts/" + p.id).toString();
        return !saved.isEmpty() && saved == fingerprint(p);
    }
    void setScriptsAllowed(const Plugin &p, bool on)
    {
        QSettings st("LuantiCreator", "LuantiCreator");
        if (on) st.setValue("pluginScripts/" + p.id, fingerprint(p));
        else st.remove("pluginScripts/" + p.id);
    }

    // ------------------------------------------------------------ modification de LuantiCreator (luanti_creator_core)
    // Autorisation distincte de celle des scripts : un plugin peut lancer des scripts sans pouvoir modifier LuantiCreator.
    // Liée au contenu du plugin, comme l'autorisation des scripts. Jamais accordée d'office, même aux plugins intégrés.
    bool coreAllowed(const Plugin &p) const
    {
        const QString saved = QSettings("LuantiCreator", "LuantiCreator").value("pluginCore/" + p.id).toString();
        return !saved.isEmpty() && saved == fingerprint(p);
    }
    void setCoreAllowed(const Plugin &p, bool on)
    {
        QSettings st("LuantiCreator", "LuantiCreator");
        if (on) st.setValue("pluginCore/" + p.id, fingerprint(p));
        else st.remove("pluginCore/" + p.id);
    }
    // vrai si le plugin PEUT actuellement modifier LuantiCreator (activé, scripts autorisés, modification autorisée)
    bool coreActive(const Plugin &p) const { return p.enabled && p.core && scriptsAllowed(p) && coreAllowed(p); }
    // noms des plugins qui peuvent altérer le comportement de LuantiCreator (pour l'avertissement permanent)
    QStringList corePlugins() const
    {
        QStringList r;
        for (const Plugin &p : list_)
            if (coreActive(p)) r << p.name;
        return r;
    }

    struct UiPatch {
        QString plugin;
        QJsonObject data;
    };
    // modifications de l'interface demandées par les scripts (startup / manual) ; main.cpp les applique
    const QVector<UiPatch> &uiPatches() const { return uiPatches_; }

    // Lance les scripts « startup » des plugins activés. Les modifications acceptées sont dans uiPatches().
    void runStartup(QStringList &msgs)
    {
        uiPatches_.clear();
        coreIcons_.clear();
        icons_.clear();
        iconOnly_.clear();
        for (const Plugin &p : list_) {
            if (!p.enabled || !scriptsAllowed(p)) continue;
            for (const PluginScript &s : p.scripts) {
                if (s.event != "startup") continue;
                const ScriptResult r = lcscript::run(p.dir, s.file, s.interpreter, baseContext(p, "startup"), QString());
                const QString pre = QObject::tr("Plugin %1, script « %2 » : ").arg(p.name, s.name);
                for (const QString &l : r.log) msgs << pre + l;
                for (const QString &w : r.warnings) msgs << pre + w;
                if (!r.ok) {
                    msgs << pre + QObject::tr("erreur : %1").arg(r.error);
                    continue;
                }
                acceptCore(p, s.name, r, msgs, true, false);
            }
        }
    }

    // Valide les demandes de modification d'un script. Rien n'est pris en compte sans autorisation explicite.
    //   uiOk  : les modifications d'interface (titre, cartes, icônes, style) sont acceptées dans cet événement
    //   luaOk : les modifications du code Lua généré sont acceptées dans cet événement
    void acceptCore(const Plugin &p, const QString &label, const ScriptResult &r, QStringList &warns, bool uiOk, bool luaOk) const
    {
        if (!r.coreUsed) return;
        const QString pre = QObject::tr("Plugin %1, script « %2 » : ").arg(p.name, label);
        if (!p.core) {
            warns << pre + QObject::tr("utilise luanti_creator_core sans déclarer « core: true » dans son manifeste : modifications ignorées");
            return;
        }
        if (!coreAllowed(p)) {
            warns << pre + QObject::tr("demande à modifier LuantiCreator mais n'y est pas autorisé (gestionnaire de plugins, case « Autoriser ce plugin à modifier LuantiCreator ») : modifications ignorées");
            return;
        }
        static const QRegularExpression keyRe("^[A-Za-z0-9_.\\-]+$");
        int lua = 0;
        for (const QJsonValue &v : r.core) {
            const QJsonObject d = v.toObject();
            const QString op = d["op"].toString();
            const bool isLua = op == "lua_prepend" || op == "lua_append" || op == "lua_replace";
            if (isLua) {
                if (!luaOk) {
                    warns << pre + QObject::tr("« %1 » ignoré : réservé aux scripts « generate » et aux scripts de fonctionnalité").arg(op);
                    continue;
                }
                if (++lua > 200 || d["code"].toString().size() > 1000000 || d["new"].toString().size() > 1000000) {
                    warns << pre + QObject::tr("trop de modifications du code Lua ou modification trop grosse : ignorée");
                    continue;
                }
                if (op == "lua_replace" && d["old"].toString().isEmpty()) continue;
                QJsonObject o = d;
                o["plugin"] = p.name;
                luaPatches_.append(o);
                continue;
            }
            if (!uiOk) {
                warns << pre + QObject::tr("« %1 » ignoré : les modifications de l'interface se font dans un script « startup » ou « manual »").arg(op);
                continue;
            }
            QJsonObject o = d;
            if (op == "window_title") {
                if (d["text"].toString().isEmpty() || d["text"].toString().size() > 200) continue;
                removeUiPatch(p.id, op);
            } else if (op == "stylesheet") {
                if (d["qss"].toString().size() > 64 * 1024) continue;
                removeUiPatch(p.id, op, d["name"].toString());
            } else if (op == "theme") {
                QJsonObject cols;
                const QJsonObject in = d["colors"].toObject();
                static const QRegularExpression hex("^#[0-9a-fA-F]{6}$");
                static const QStringList names = {"bg", "surface", "surface2", "border", "text", "muted", "accent", "accentHover",
                                                  "accentText", "hover", "scroll", "danger", "disabled", "placeholder"};
                for (auto it = in.begin(); it != in.end(); ++it) {
                    if (names.contains(it.key()) && hex.match(it.value().toString()).hasMatch()) cols[it.key()] = it.value();
                    else warns << pre + QObject::tr("couleur « %1 » refusée").arg(it.key());
                }
                if (cols.isEmpty()) continue;
                o["colors"] = cols;
            } else if (op == "font") {
                const double sz = d["size"].toDouble(0);
                if (d.contains("size") && (sz < 6 || sz > 32)) continue;
                if (d["family"].toString().size() > 100) continue;
            } else if (op == "card") {
                if (d["card"].toString().isEmpty()) continue;
            } else if (op == "icon") {
                const QString key = d["key"].toString(), rel = d["file"].toString();
                const QString suf = QFileInfo(rel).suffix().toLower();
                if (!keyRe.match(key).hasMatch() || !lcscript::safeRel(rel) ||
                    !(suf == "png" || suf == "jpg" || suf == "jpeg" || suf == "bmp")) {
                    warns << pre + QObject::tr("icône « %1 » refusée (clé ou chemin invalide)").arg(key);
                    continue;
                }
                const QString abs = p.dir + "/" + rel;
                if (!QFile::exists(abs)) {
                    warns << pre + QObject::tr("icône « %1 » : fichier introuvable (%2)").arg(key, rel);
                    continue;
                }
                coreIcons_[key] = qMakePair(abs, d["icon_only"].toBool(false));
                icons_.clear();
                iconOnly_.clear();
            } else {
                warns << pre + QObject::tr("modification inconnue « %1 » ignorée").arg(op);
                continue;
            }
            UiPatch up;
            up.plugin = p.id;
            up.data = o;
            uiPatches_.append(up);
        }
    }

    // Applique les modifications du code Lua généré (demandées pendant la génération qui vient d'avoir lieu).
    QString patchLua(QString lua, QStringList &warns) const
    {
        if (luaPatches_.isEmpty()) return lua;
        QStringList who;
        for (const QJsonObject &d : luaPatches_) {
            const QString op = d["op"].toString();
            if (op == "lua_prepend") {
                lua = d["code"].toString() + (d["code"].toString().endsWith('\n') ? "" : "\n") + lua;
            } else if (op == "lua_append") {
                if (!lua.endsWith('\n')) lua += "\n";
                lua += d["code"].toString() + (d["code"].toString().endsWith('\n') ? "" : "\n");
            } else if (op == "lua_replace") {
                const QString old = d["old"].toString();
                const int count = d["count"].toInt(0);
                int done = 0, from = 0;
                for (;;) {
                    if (count > 0 && done >= count) break;
                    const int i = lua.indexOf(old, from);
                    if (i < 0) break;
                    lua.replace(i, old.size(), d["new"].toString());
                    from = i + d["new"].toString().size();
                    ++done;
                }
                if (done == 0)
                    warns << QObject::tr("Plugin %1 : lua_replace n'a rien trouvé à remplacer").arg(d["plugin"].toString());
            }
            if (!who.contains(d["plugin"].toString())) who << d["plugin"].toString();
        }
        return QObject::tr("-- Attention : ce fichier a été modifié par le(s) plugin(s) : %1 (luanti_creator_core)\n").arg(who.join(", ")) + lua;
    }

    // exécute un script « manual » ; les éléments à ajouter au projet sont dans res.addItems
    bool runManual(const QString &pluginId, const QString &scriptId, ScriptResult &res, QString &err) const
    {
        const Plugin *p = find(pluginId);
        if (!p) {
            err = QObject::tr("Plugin introuvable.");
            return false;
        }
        for (const PluginScript &s : p->scripts) {
            if (s.id != scriptId) continue;
            if (!scriptsAllowed(*p)) {
                err = QObject::tr("Les scripts de ce plugin ne sont pas autorisés (cochez « Autoriser les scripts »).");
                return false;
            }
            res = lcscript::run(p->dir, s.file, s.interpreter, baseContext(*p, "manual"), QString());
            if (!res.ok) err = res.error;
            else acceptCore(*p, s.name, res, res.warnings, true, false);
            return res.ok;
        }
        err = QObject::tr("Script introuvable.");
        return false;
    }

    // ------------------------------------------------------------ installation
    // Lit l'archive sans rien extraire.
    bool inspectZip(const QString &zipPath, ZipPluginInfo &info, QString &err) const
    {
        std::string z;
        std::vector<lczip::Entry> files;
        if (!loadZipFiles(zipPath, z, files, &err)) return false;
        std::string prefix;
        std::string manifest;
        if (!lczip::findPluginRoot(files, prefix, &manifest)) {
            err = QObject::tr("Cette archive n'est pas un plugin LuantiCreator (plugin.yaml ou plugin.xml introuvable).");
            return false;
        }
        info.prefix = QString::fromStdString(prefix);
        std::string text, e;
        for (const lczip::Entry &en : files)
            if (en.name == prefix + manifest) {
                if (!lczip::readEntry(z, en, text, e)) {
                    err = QString::fromStdString(e);
                    return false;
                }
                break;
            }
        QJsonObject o;
        if (!lcman::parse(QByteArray::fromStdString(text), QString::fromStdString(manifest), o, err)) return false;
        info.id = lczip::sanitizeName(o["id"].toString().toStdString()).c_str();
        if (info.id.isEmpty()) {
            err = QObject::tr("Le champ « id » du manifeste est obligatoire (a-z, 0-9, _).");
            return false;
        }
        info.name = o["name"].toString(info.id);
        info.version = o["version"].toString();
        info.author = o["author"].toString();
        info.description = o["description"].toString();
        info.blocks = o["blocks"].toArray().size();
        for (const QJsonValue &v : o["blocksets"].toArray()) info.blocks += v.toObject()["names"].toArray().size();
        info.icons = o["icons"].toObject().size();
        info.presets = o["presets"].toArray().size();
        info.snippets = o["snippets"].toArray().size();
        info.features = o["features"].toArray().size();
        info.scripts = o["scripts"].toArray().size();
        for (const QJsonValue &fv : o["features"].toArray())
            if (!fv.toObject()["script"].toString().isEmpty()) ++info.scripts;
        for (const lczip::Entry &en : files)
            if (en.name.compare(0, prefix.size(), prefix) == 0 && allowedFile(QString::fromStdString(en.name.substr(prefix.size()))))
                ++info.files;
            else if (en.name.compare(0, prefix.size(), prefix) == 0)
                ++info.skipped;
        info.exists = QDir(root() + "/" + info.id).exists();
        return true;
    }

    bool installZip(const QString &zipPath, QString &installedId, QString &err)
    {
        ZipPluginInfo info;
        if (!inspectZip(zipPath, info, err)) return false;
        std::string z;
        std::vector<lczip::Entry> files;
        if (!loadZipFiles(zipPath, z, files, &err)) return false;
        const std::string prefix = info.prefix.toStdString();
        QDir().mkpath(root());
        const QString tmp = root() + "/.install_" + info.id;
        QDir(tmp).removeRecursively();
        QDir().mkpath(tmp);
        quint64 total = 0;
        int count = 0;
        for (const lczip::Entry &en : files) {
            if (en.name.compare(0, prefix.size(), prefix) != 0) continue;
            const QString rel = QString::fromStdString(en.name.substr(prefix.size()));
            if (rel.isEmpty() || !allowedFile(rel)) continue;
            if (++count > 600) {
                err = QObject::tr("Plugin trop gros (plus de 600 fichiers).");
                QDir(tmp).removeRecursively();
                return false;
            }
            std::string data, e;
            if (en.usize > 8u * 1024u * 1024u || !lczip::readEntry(z, en, data, e)) {
                err = e.empty() ? QObject::tr("Fichier trop volumineux dans le plugin : %1").arg(rel) : QString::fromStdString(e);
                QDir(tmp).removeRecursively();
                return false;
            }
            total += data.size();
            if (total > 40ull * 1024 * 1024) {
                err = QObject::tr("Plugin trop volumineux (plus de 40 Mo décompressés).");
                QDir(tmp).removeRecursively();
                return false;
            }
            const QString path = QDir::cleanPath(tmp + "/" + rel);
            if (!path.startsWith(tmp + "/")) {
                err = QObject::tr("Chemin dangereux dans le plugin : %1").arg(rel);
                QDir(tmp).removeRecursively();
                return false;
            }
            QDir().mkpath(QFileInfo(path).absolutePath());
            QFile out(path);
            if (!out.open(QIODevice::WriteOnly) || out.write(data.data(), qint64(data.size())) != qint64(data.size())) {
                err = QObject::tr("Impossible d'écrire %1").arg(path);
                QDir(tmp).removeRecursively();
                return false;
            }
        }
        const QString dest = root() + "/" + info.id;
        QDir(dest).removeRecursively();
        if (!QDir().rename(tmp, dest)) {
            err = QObject::tr("Impossible d'installer le plugin dans %1").arg(dest);
            QDir(tmp).removeRecursively();
            return false;
        }
        installedId = info.id;
        reload();
        return true;
    }

    // Plugins intégrés à l'exécutable : les .zip du dossier plugins/ au moment de la compilation (ressources « :/lcplugins »).
    // Un plugin est (ré)installé quand son archive est nouvelle ou a changé ; il n'est PAS réinstallé si vous l'avez
    // supprimé (jusqu'à la prochaine version de l'archive). Un plugin du même identifiant que vous avez installé vous-même
    // n'est jamais écrasé. Les scripts d'un plugin intégré sont autorisés d'office (ils font partie de votre exécutable).
    // Renvoie le nombre de plugins installés ou mis à jour.
    int installBundled(QStringList *messages = nullptr)
    {
        const QDir res(":/lcplugins");
        const QStringList names = res.entryList({"*.zip"}, QDir::Files, QDir::Name);
        if (names.isEmpty()) return 0;
        QSettings st("LuantiCreator", "LuantiCreator");
        QTemporaryDir td;
        if (!td.isValid()) return 0;
        int n = 0;
        for (const QString &name : names) {
            QFile rf(":/lcplugins/" + name);
            if (!rf.open(QIODevice::ReadOnly)) continue;
            const QByteArray data = rf.readAll();
            const QString hash = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
            const QString tmpZip = td.path() + "/" + name;
            {
                QFile f(tmpZip);
                if (!f.open(QIODevice::WriteOnly) || f.write(data) != data.size()) continue;
            }
            ZipPluginInfo info;
            QString err;
            if (!inspectZip(tmpZip, info, err)) {
                if (messages) *messages << QObject::tr("Plugin intégré « %1 » ignoré : %2").arg(name, err);
                continue;
            }
            const QString key = "bundled/" + info.id;
            if (st.value(key).toString() == hash) continue; // déjà installé (ou supprimé volontairement)
            const QString dest = root() + "/" + info.id;
            if (QDir(dest).exists() && !QFile::exists(dest + "/.bundled")) {
                if (messages) *messages << QObject::tr("Plugin intégré « %1 » ignoré : un plugin « %2 » installé par vous existe déjà.").arg(name, info.id);
                continue;
            }
            QString id;
            if (!installZip(tmpZip, id, err)) {
                if (messages) *messages << QObject::tr("Plugin intégré « %1 » : %2").arg(name, err);
                continue;
            }
            QFile mark(root() + "/" + id + "/.bundled");
            if (mark.open(QIODevice::WriteOnly)) mark.write(hash.toLatin1());
            mark.close();
            st.setValue(key, hash);
            reload();
            if (const Plugin *ip = find(id))
                if (ip->hasScripts()) setScriptsAllowed(*ip, true);
            ++n;
        }
        if (n) reload();
        return n;
    }

    bool remove(const QString &id)
    {
        const Plugin *p = find(id);
        if (!p) return false;
        const bool ok = QDir(p->dir).removeRecursively();
        QSettings("LuantiCreator", "LuantiCreator").remove("pluginScripts/" + id);
        reload();
        return ok;
    }

    bool exportZip(const QString &id, const QString &zipPath, QString &err) const
    {
        const Plugin *p = find(id);
        if (!p) {
            err = QObject::tr("Plugin introuvable.");
            return false;
        }
        QString e;
        if (!writeZip(zipPath, p->dir, p->id, &e)) {
            err = e;
            return false;
        }
        return true;
    }

    // Crée un plugin d'exemple modifiable dans le dossier des plugins.
    bool createSkeleton(const QString &rawId, const QString &name, QString &id, QString &err, const QString &format = "yaml",
                        bool empty = false)
    {
        id = QString::fromStdString(lczip::sanitizeName(rawId.toStdString()));
        if (id.isEmpty()) {
            err = QObject::tr("Identifiant invalide (a-z, 0-9, _).");
            return false;
        }
        const QString dir = root() + "/" + id;
        if (QDir(dir).exists()) {
            err = QObject::tr("Un plugin « %1 » existe déjà.").arg(id);
            return false;
        }
        QDir().mkpath(dir + "/icons");
        QDir().mkpath(dir + "/textures");
        QDir().mkpath(dir + "/lua");
        QDir().mkpath(dir + "/templates");
        auto put = [&](const QString &rel, const QByteArray &data) {
            QFile f(dir + "/" + rel);
            if (!f.open(QIODevice::WriteOnly)) return false;
            f.write(data);
            return true;
        };
        QJsonObject o;
        o["format"] = 1;
        o["id"] = id;
        o["name"] = name.isEmpty() ? id : name;
        o["version"] = "1.0";
        o["author"] = "";
        o["description"] = empty ? "" : "Plugin créé avec LuantiCreator.";
        if (!empty) { // plugin d'exemple ; « empty » : manifeste vide, on part de zéro
        o["icons"] = QJsonObject();
        o["blocks"] = QJsonArray();
        QJsonObject preset;
        preset["tab"] = "biomes";
        preset["name"] = "Biome exemple";
        preset["values"] = QJsonObject{{"node_top", "default:dirt_with_grass"}, {"node_filler", "default:dirt"},
                                       {"node_stone", "default:stone"}, {"heat", 50}, {"humid", 50}};
        o["presets"] = QJsonArray{preset};
        QJsonObject sn;
        sn["id"] = "bienvenue";
        sn["name"] = "Message de bienvenue";
        sn["description"] = "Affiche un message quand un joueur rejoint.";
        sn["file"] = "lua/bienvenue.lua";
        o["snippets"] = QJsonArray{sn};
        QJsonObject ft;
        ft["id"] = "salutations";
        ft["title"] = "Salutations";
        ft["description"] = "Commande de chat qui dit bonjour (exemple de fonctionnalité).";
        ft["prefix"] = "salut_";
        ft["template"] = "templates/salutations.lua";
        ft["fields"] = QJsonArray{
            QJsonObject{{"key", "name"}, {"label", "Nom de la commande"}, {"type", "text"}},
            QJsonObject{{"key", "texte"}, {"label", "Texte"}, {"type", "text"}, {"default", "Bonjour !"}},
            QJsonObject{{"key", "fois"}, {"label", "Répétitions"}, {"type", "int"}, {"default", 1}, {"min", 1}, {"max", 10}},
            QJsonObject{{"key", "bruyant"}, {"label", "En majuscules"}, {"type", "check"}}};
        o["features"] = QJsonArray{ft};
        }
        bool ok = put(format == "xml" ? "plugin.xml" : "plugin.yaml", lcman::serialize(o, format == "xml" ? "plugin.xml" : "plugin.yaml"));
        if (!empty) {
        ok = ok && put("lua/bienvenue.lua",
                       "core.register_on_joinplayer(function(player)\n"
                       "    core.chat_send_player(player:get_player_name(), \"Bienvenue !\")\n"
                       "end)\n");
        ok = ok && put("templates/salutations.lua",
                       "core.register_chatcommand({{name|lua}}, {\n"
                       "    description = \"Dit bonjour\",\n"
                       "    func = function(name)\n"
                       "        local msg = string.rep({{texte|lua}} .. \" \", {{fois|num}})\n"
                       "{{#if bruyant}}\n"
                       "        msg = msg:upper()\n"
                       "{{/if}}\n"
                       "        return true, msg\n"
                       "    end,\n"
                       "})\n");
        ok = ok && put("README.txt",
                       "Plugin LuantiCreator. Ajoutez des icônes (icons/), des textures (textures/), des blocs, "
                       "des modèles prédéfinis, des extraits Lua et des fonctionnalités dans plugin.yaml (ou plugin.xml).\n"
                       "Documentation : PLUGINS.md\n");
        }
        if (!ok) {
            err = QObject::tr("Impossible d'écrire dans %1").arg(dir);
            return false;
        }
        reload();
        return true;
    }

    // ------------------------------------------------------------ lecture d'un plugin
    // Remplit p depuis le manifeste de dir (yaml / xml) ; les erreurs non fatales vont dans p.problems.
    static bool loadOne(const QString &dir, Plugin &p)
    {
        p.dir = dir;
        const QString mf = lcman::find(dir);
        QFile f(dir + "/" + mf);
        if (mf.isEmpty() || !f.open(QIODevice::ReadOnly)) {
            p.problems << QObject::tr("manifeste (plugin.yaml / plugin.xml) introuvable ou illisible");
            return false;
        }
        QJsonObject o;
        QString perr;
        if (!lcman::parse(f.readAll(), mf, o, perr)) {
            p.problems << perr;
            return false;
        }
        p.id = QString::fromStdString(lczip::sanitizeName(o["id"].toString().toStdString()));
        if (p.id.isEmpty()) p.problems << QObject::tr("« id » manquant ou invalide");
        p.name = o["name"].toString();
        p.version = o["version"].toString();
        p.author = o["author"].toString();
        p.description = o["description"].toString();
        p.core = o["core"].toBool(false);
        if (o["format"].toInt(1) != 1) p.problems << QObject::tr("format %1 inconnu (attendu : 1)").arg(o["format"].toInt());

        auto path = [&](const QString &rel, const QString &what) -> QString {
            if (rel.isEmpty()) return QString();
            if (!relOk(rel)) {
                p.problems << QObject::tr("%1 : chemin non autorisé « %2 »").arg(what, rel);
                return QString();
            }
            const QString abs = QDir::cleanPath(dir + "/" + rel);
            if (!QFile::exists(abs)) {
                p.problems << QObject::tr("%1 : fichier introuvable « %2 »").arg(what, rel);
                return QString();
            }
            return abs;
        };
        auto isPng = [&](const QString &abs, const QString &what) {
            if (abs.isEmpty()) return false;
            if (QImage(abs).isNull()) {
                p.problems << QObject::tr("%1 : image illisible « %2 »").arg(what, QFileInfo(abs).fileName());
                return false;
            }
            return true;
        };

        // icônes
        const QJsonObject ic = o["icons"].toObject();
        for (auto it = ic.begin(); it != ic.end(); ++it) {
            const QString key = it.key();
            if (!lcIconKeyExists(key))
                p.problems << QObject::tr("icône « %1 » : clé inconnue (attendu : app, card.*, cat.*, btn.*, <contexte>.btn.*, ou une clé d'icône de l'application)").arg(key);
            // forme simple : "cle": "icons/x.png" — forme objet : "cle": {"file": "icons/x.png", "icon_only": true}
            QString file;
            bool only = false;
            if (it.value().isObject()) {
                const QJsonObject io = it.value().toObject();
                file = io["file"].toString();
                only = io["icon_only"].toBool(false);
            } else {
                file = it.value().toString();
            }
            const QString abs = path(file, QObject::tr("icône « %1 »").arg(key));
            if (isPng(abs, QObject::tr("icône « %1 »").arg(key))) {
                p.icons[key] = abs;
                if (only) p.iconOnly.insert(key);
            }
        }

        // textures qui remplacent / complètent celles d'une référence
        const QJsonObject tx = o["textures"].toObject();
        for (auto it = tx.begin(); it != tx.end(); ++it) {
            if (!it.key().contains(':')) {
                p.problems << QObject::tr("texture « %1 » : la référence doit être de la forme mod:nom").arg(it.key());
                continue;
            }
            const QString abs = path(it.value().toString(), QObject::tr("texture « %1 »").arg(it.key()));
            if (isPng(abs, QObject::tr("texture « %1 »").arg(it.key()))) p.textures[it.key()] = abs;
        }

        // blocs
        auto addBlock = [&](const QString &ref, const QString &label, const QString &cat, const QString &tex) {
            if (!ref.contains(':') || ref.startsWith(':') || ref.endsWith(':')) {
                p.problems << QObject::tr("bloc « %1 » : la référence doit être de la forme mod:nom").arg(ref);
                return;
            }
            PluginBlock b;
            b.ref = ref;
            b.label = label.isEmpty() ? ref : label;
            b.category = cat.isEmpty() ? (p.name.isEmpty() ? p.id : p.name) : cat;
            if (!tex.isEmpty()) {
                const QString abs = path(tex, QObject::tr("bloc « %1 »").arg(ref));
                if (isPng(abs, QObject::tr("bloc « %1 »").arg(ref))) b.texture = abs;
            }
            p.blocks.append(b);
        };
        for (const QJsonValue &v : o["blocks"].toArray()) {
            const QJsonObject b = v.toObject();
            addBlock(b["ref"].toString().trimmed(), b["label"].toString(), b["category"].toString(), b["texture"].toString());
        }
        for (const QJsonValue &v : o["blocksets"].toArray()) {
            const QJsonObject b = v.toObject();
            const QString refPat = b["ref"].toString(), labelPat = b["label"].toString("{name}"), texPat = b["texture"].toString();
            if (refPat.isEmpty()) {
                p.problems << QObject::tr("blocksets : « ref » manquant (ex : \"wool:{name}\")");
                continue;
            }
            for (const QJsonValue &nv : b["names"].toArray()) {
                const QString n = nv.toString();
                auto sub = [&](QString s) { return s.replace("{name}", n); };
                addBlock(sub(refPat), sub(labelPat), b["category"].toString(), texPat.isEmpty() ? QString() : sub(texPat));
            }
        }
        if (p.blocks.size() > 5000) {
            p.problems << QObject::tr("trop de blocs (maximum 5000)");
            p.blocks.resize(5000);
        }

        // modèles prédéfinis
        for (const QJsonValue &v : o["presets"].toArray()) {
            const QJsonObject b = v.toObject();
            PluginPreset s;
            s.tab = b["tab"].toString();
            s.name = b["name"].toString();
            s.values = b["values"].toObject().toVariantMap();
            if (s.name.isEmpty()) {
                p.problems << QObject::tr("modèle prédéfini sans nom");
                continue;
            }
            s.values.remove("name"); // le nom technique est attribué automatiquement
            p.presets.append(s);
        }

        // extraits Lua
        for (const QJsonValue &v : o["snippets"].toArray()) {
            const QJsonObject b = v.toObject();
            PluginSnippet s;
            s.id = QString::fromStdString(lczip::sanitizeName(b["id"].toString().toStdString()));
            s.name = b["name"].toString(s.id);
            s.desc = b["description"].toString();
            s.file = b["file"].toString();
            s.tpl = b["template"].toBool(false);
            s.onByDefault = b["default"].toBool(false);
            if (s.id.isEmpty()) {
                p.problems << QObject::tr("extrait sans « id »");
                continue;
            }
            const QString abs = path(s.file, QObject::tr("extrait « %1 »").arg(s.id));
            if (abs.isEmpty()) continue;
            QFile lf(abs);
            if (!lf.open(QIODevice::ReadOnly) || lf.size() > 1024 * 1024) {
                p.problems << QObject::tr("extrait « %1 » : fichier illisible ou trop gros").arg(s.id);
                continue;
            }
            s.text = QString::fromUtf8(lf.readAll());
            if (s.tpl) {
                std::vector<lctpl::Node> nodes;
                std::string err;
                if (!lctpl::parse(s.text.toStdString(), nodes, err))
                    p.problems << QObject::tr("extrait « %1 » : gabarit invalide : %2").arg(s.id, QString::fromStdString(err));
            }
            p.snippets.append(s);
        }

        // fonctionnalités
        QSet<QString> featIds;
        for (const QJsonValue &v : o["features"].toArray()) {
            const QJsonObject b = v.toObject();
            PluginFeature ft;
            ft.id = QString::fromStdString(lczip::sanitizeName(b["id"].toString().toStdString()));
            if (ft.id.isEmpty() || featIds.contains(ft.id)) {
                p.problems << QObject::tr("fonctionnalité sans « id » ou en double");
                continue;
            }
            featIds.insert(ft.id);
            ft.key = "plugin." + p.id + "." + ft.id;
            ft.title = b["title"].toString(ft.id);
            ft.desc = b["description"].toString();
            ft.icon = b["icon"].toString();
            ft.iconOnly = b["icon_only"].toBool(false);
            ft.prefix = b["prefix"].toString(ft.id + "_");
            ft.tplFile = b["template"].toString();
            ft.script = b["script"].toString();
            ft.scriptInterpreter = b["interpreter"].toString();
            if (!ft.script.isEmpty()) {
                const QString sabs = path(ft.script, QObject::tr("fonctionnalité « %1 » : script").arg(ft.id));
                if (sabs.isEmpty()) continue;
                if (!lcscript::supportedExt(QFileInfo(ft.script).suffix().toLower())) {
                    p.problems << QObject::tr("fonctionnalité « %1 » : type de script non pris en charge (py, sh, js, rb, lua, pl)").arg(ft.id);
                    continue;
                }
            }
            if (!ft.tplFile.isEmpty() || ft.script.isEmpty()) { // le gabarit est facultatif quand un script génère le code
                const QString abs = path(ft.tplFile, QObject::tr("fonctionnalité « %1 » : gabarit").arg(ft.id));
                if (abs.isEmpty()) continue;
                QFile tf(abs);
                if (!tf.open(QIODevice::ReadOnly) || tf.size() > 1024 * 1024) {
                    p.problems << QObject::tr("fonctionnalité « %1 » : gabarit illisible ou trop gros").arg(ft.id);
                    continue;
                }
                ft.tplText = QString::fromUtf8(tf.readAll());
                std::vector<lctpl::Node> nodes;
                std::string err;
                if (!lctpl::parse(ft.tplText.toStdString(), nodes, err)) {
                    p.problems << QObject::tr("fonctionnalité « %1 » : gabarit invalide : %2").arg(ft.id, QString::fromStdString(err));
                    continue;
                }
            }
            QSet<QString> keys;
            bool hasName = false;
            for (const QJsonValue &fv : b["fields"].toArray()) {
                Field fd;
                QString ferr;
                if (!parseField(fv.toObject(), fd, ferr)) {
                    p.problems << QObject::tr("fonctionnalité « %1 » : %2").arg(ft.id, ferr);
                    continue;
                }
                if (keys.contains(fd.key)) {
                    p.problems << QObject::tr("fonctionnalité « %1 » : champ « %2 » en double").arg(ft.id, fd.key);
                    continue;
                }
                keys.insert(fd.key);
                if (fd.key == "name") hasName = true;
                ft.fields.append(fd);
            }
            if (!hasName) {
                Field nf;
                nf.key = "name";
                nf.label = QObject::tr("Nom technique");
                nf.type = TText;
                ft.fields.prepend(nf);
            }
            if (ft.fields.size() > 60) {
                p.problems << QObject::tr("fonctionnalité « %1 » : trop de champs (maximum 60)").arg(ft.id);
                continue;
            }
            if (!ft.icon.isEmpty()) {
                const QString iabs = path(ft.icon, QObject::tr("fonctionnalité « %1 » : icône").arg(ft.id));
                if (isPng(iabs, QObject::tr("fonctionnalité « %1 » : icône").arg(ft.id))) {
                    p.icons["card." + ft.key] = iabs;
                    if (ft.iconOnly) p.iconOnly.insert("card." + ft.key);
                }
            }
            p.features.append(ft);
        }

        // scripts du plugin
        QSet<QString> scriptIds;
        for (const QJsonValue &v : o["scripts"].toArray()) {
            const QJsonObject b = v.toObject();
            PluginScript s;
            s.id = QString::fromStdString(lczip::sanitizeName(b["id"].toString().toStdString()));
            s.name = b["name"].toString(s.id);
            s.desc = b["description"].toString();
            s.file = b["file"].toString();
            s.interpreter = b["interpreter"].toString();
            s.event = b["event"].toString("manual");
            if (s.id.isEmpty() || scriptIds.contains(s.id)) {
                p.problems << QObject::tr("script sans « id » ou en double");
                continue;
            }
            if (s.event != "manual" && s.event != "generate" && s.event != "startup") {
                p.problems << QObject::tr("script « %1 » : « event » doit valoir manual, generate ou startup").arg(s.id);
                continue;
            }
            if (path(s.file, QObject::tr("script « %1 »").arg(s.id)).isEmpty()) continue;
            if (!lcscript::supportedExt(QFileInfo(s.file).suffix().toLower())) {
                p.problems << QObject::tr("script « %1 » : type non pris en charge (py, sh, js, rb, lua, pl)").arg(s.id);
                continue;
            }
            scriptIds.insert(s.id);
            p.scripts.append(s);
        }

        // modèles prédéfinis : le nom d'une fonctionnalité du plugin désigne sa page
        for (PluginPreset &s : p.presets) {
            if (featIds.contains(s.tab)) s.tab = "plugin." + p.id + "." + s.tab;
            else if (!pluginTabKeys().contains(s.tab) && !s.tab.startsWith("plugin."))
                p.problems << QObject::tr("modèle « %1 » : page « %2 » inconnue").arg(s.name, s.tab);
        }
        return true;
    }

    // Fichiers acceptés dans un plugin (pas de binaires exécutables, pas de scripts autres que Lua).
    static bool allowedFile(const QString &rel)
    {
        if (!relOk(rel)) return false;
        const QString ext = QFileInfo(rel).suffix().toLower();
        return ext == "py" || ext == "sh" || ext == "js" || ext == "rb" || ext == "pl" || ext == "png" || ext == "yaml" || ext == "yml" || ext == "xml" || ext == "json" || ext == "lua" || ext == "tpl" || ext == "txt" || ext == "md";
    }

    static bool relOk(const QString &rel)
    {
        if (rel.isEmpty() || rel.startsWith('/') || rel.contains('\\') || rel.contains(':')) return false;
        for (const QString &seg : rel.split('/'))
            if (seg == "..") return false;
        return true;
    }

    // ------------------------------------------------------------ rendu d'une fonctionnalité
    QString renderFeature(const Plugin &p, const PluginFeature &f, const QList<QVariantMap> &items, const QString &modid,
                          const QString &root, QStringList &warns) const
    {
        QString out;
        int index = 0;
        for (const QVariantMap &m : items) {
            ++index;
            lctpl::Vars vars;
            vars["modname"] = lctpl::Value::str(modid.toStdString());
            vars["index"] = lctpl::Value::number(index);
            const QString nameRaw = m.value("name").toString();
            const QString nameId = identOf(nameRaw);
            for (const Field &fd : f.fields) {
                const QVariant v = m.value(fd.key);
                lctpl::Value val;
                switch (fd.type) {
                case TInt: case TDouble: val = lctpl::Value::number(v.toDouble()); break;
                case TCheck: val = lctpl::Value::boolean(v.toBool()); break;
                case TFile: {
                    const QString src = v.toString();
                    QString dst;
                    if (!src.isEmpty() && QFile::exists(src)) {
                        const QString ext = QFileInfo(src).suffix().toLower();
                        const QString sub = (ext == "png") ? "textures" : "models";
                        dst = (ext == "png") ? modid + "_" + nameId + "_" + fd.key + ".png" : QFileInfo(src).fileName();
                        if (!root.isEmpty()) {
                            QDir().mkpath(root + "/" + sub);
                            QFile::remove(root + "/" + sub + "/" + dst);
                            if (!QFile::copy(src, root + "/" + sub + "/" + dst))
                                warns << QObject::tr("Plugin %1 : impossible de copier %2").arg(p.name, src);
                        }
                    } else if (!src.isEmpty()) {
                        warns << QObject::tr("Plugin %1 : fichier introuvable : %2").arg(p.name, src);
                    }
                    val = lctpl::Value::str(dst.toStdString());
                    break;
                }
                default: val = lctpl::Value::str(v.toString().toStdString()); break;
                }
                vars[fd.key.toStdString()] = val;
            }
            vars["name"] = lctpl::Value::str(nameId.toStdString());
            vars["name_raw"] = lctpl::Value::str(nameRaw.toStdString());
            vars["id"] = lctpl::Value::str((modid + ":" + nameId).toStdString());
            std::string res, err;
            std::vector<std::string> w;
            if (!lctpl::render(f.tplText.toStdString(), vars, res, err, &w)) {
                warns << QObject::tr("Plugin %1, « %2 » (%3) : %4").arg(p.name, f.title, nameRaw, QString::fromStdString(err));
                continue;
            }
            for (const std::string &x : w)
                warns << QObject::tr("Plugin %1, « %2 » : %3").arg(p.name, f.title, QString::fromStdString(x));
            out += QString::fromStdString(res);
            if (!out.endsWith('\n')) out += "\n";
            out += "\n";
        }
        return out;
    }

    static bool parseField(const QJsonObject &o, Field &f, QString &err)
    {
        f.key = o["key"].toString();
        if (f.key.isEmpty() || f.key != identOf(f.key)) {
            err = QObject::tr("clé de champ invalide « %1 » (a-z, 0-9, _)").arg(f.key);
            return false;
        }
        static const QMap<QString, FieldType> types = {
            {"text", TText}, {"int", TInt}, {"double", TDouble}, {"combo", TCombo}, {"file", TFile},
            {"check", TCheck}, {"multi", TMulti}, {"ref", TRef}, {"reflist", TRefList}};
        const QString t = o["type"].toString("text");
        if (!types.contains(t)) {
            err = QObject::tr("champ « %1 » : type inconnu « %2 »").arg(f.key, t);
            return false;
        }
        f.type = types.value(t);
        f.label = o["label"].toString(f.key);
        for (const QJsonValue &v : o["options"].toArray()) f.opts << v.toString();
        if (f.type == TCombo && f.opts.isEmpty()) {
            err = QObject::tr("champ « %1 » : un champ « combo » a besoin de « options »").arg(f.key);
            return false;
        }
        const QJsonValue d = o["default"];
        if (d.isBool()) f.def = d.toBool() ? "1" : "0";
        else if (d.isDouble()) f.def = QString::number(d.toDouble());
        else f.def = d.toString();
        f.mn = o["min"].toInt(f.type == TInt || f.type == TDouble ? 0 : 0);
        f.mx = o["max"].toInt(9999);
        if (f.mx < f.mn) f.mx = f.mn;
        if (o.contains("filter")) f.filter = o["filter"].toString();
        f.hint = o["hint"].toString();
        return true;
    }

private:
    QJsonObject baseContext(const Plugin &p, const QString &event) const
    {
        QJsonObject c = ctx_;
        c["event"] = event;
        QJsonObject po;
        po["id"] = p.id;
        po["name"] = p.name;
        po["version"] = p.version;
        po["dir"] = p.dir;
        c["plugin"] = po;
        c["core_allowed"] = p.core && scriptsAllowed(p) && coreAllowed(p); // le script sait s'il peut modifier LuantiCreator
        return c;
    }
    static QJsonArray itemsJson(const QList<QVariantMap> &items, const QString &modid)
    {
        QJsonArray a;
        for (const QVariantMap &m : items) {
            QJsonObject o = QJsonObject::fromVariantMap(m);
            const QString ident = identOf(m.value("name").toString());
            o["_ident"] = ident;
            o["_ref"] = modid + ":" + ident;
            a.append(o);
        }
        return a;
    }
    // exécute un script pendant la génération et renvoie le Lua à insérer
    QString scriptOutput(const Plugin &p, const QString &label, const QString &file, const QString &interp,
                         QJsonObject ctx, const QString &root, QStringList &warns) const
    {
        const QString head = "-- Plugin « " + p.name + " » : " + label + " (script)\n";
        if (root.isEmpty()) // aperçu : on n'exécute rien (le script tourne à l'export et au lancement du serveur de test)
            return head + "-- (résultat visible après « Tester » ou « Exporter » : les scripts ne tournent pas dans l'aperçu)\n\n";
        if (!scriptsAllowed(p)) {
            warns << QObject::tr("Plugin %1, script « %2 » ignoré : scripts non autorisés (gestionnaire de plugins, case « Autoriser les scripts »).").arg(p.name, label);
            return head + "-- ignoré : scripts non autorisés\n\n";
        }
        const ScriptResult r = lcscript::run(p.dir, file, interp, ctx, root);
        const QString pre = QObject::tr("Plugin %1, script « %2 » : ").arg(p.name, label);
        for (const QString &l : r.log) warns << pre + l;
        for (const QString &w : r.warnings) warns << pre + w;
        if (!r.addItems.isEmpty()) warns << pre + QObject::tr("add_item() est ignoré pendant la génération (réservé aux scripts manuels)");
        if (!r.ok) {
            warns << pre + QObject::tr("erreur : %1").arg(r.error);
            return head + "-- erreur dans le script (voir les avertissements)\n\n";
        }
        acceptCore(p, label, r, warns, false, true);
        QString lua = r.lua;
        if (!lua.isEmpty() && !lua.endsWith('\n')) lua += "\n";
        return head + lua + "\n";
    }

    void removeUiPatch(const QString &plugin, const QString &op, const QString &name = QString()) const
    {
        for (int i = uiPatches_.size() - 1; i >= 0; --i)
            if (uiPatches_[i].plugin == plugin && uiPatches_[i].data["op"].toString() == op &&
                (op != "stylesheet" || uiPatches_[i].data["name"].toString() == name))
                uiPatches_.removeAt(i);
    }

    QJsonObject ctx_;
    QVector<Plugin> list_;
    mutable QHash<QString, QIcon> icons_;
    mutable QHash<QString, bool> iconOnly_;
    mutable QHash<QString, QPair<QString, bool>> coreIcons_; // clé -> (fichier, icône seule) : imposé par luanti_creator_core
    mutable QVector<UiPatch> uiPatches_;
    mutable QVector<QJsonObject> luaPatches_;
};

inline PluginHost &plugins() { return PluginHost::I(); }

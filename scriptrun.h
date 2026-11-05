#pragma once
// Exécution des scripts de plugins (Python, shell, Node, Lua, Perl, Ruby).
//
// ATTENTION : un script est un vrai programme, lancé avec les droits de l'utilisateur. LuantiCreator ne l'exécute que
// si l'utilisateur a explicitement autorisé les scripts du plugin (voir PluginHost::scriptsAllowed).
//
// Protocole (voir PLUGINS.md) :
//   - le contexte (JSON) est écrit dans un fichier ; son chemin est dans la variable d'environnement LC_CONTEXT ;
//   - le script écrit son résultat dans LC_RESULT (JSON) et ses fichiers dans LC_OUT_DIR ;
//   - la bibliothèque Python « luanti_creator » (embarquée) fait tout cela automatiquement ;
//   - sans fichier résultat, la sortie standard du script est prise comme code Lua (ou comme journal pour un script manuel).

#include <QtWidgets>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "lcsdk.h"

struct ScriptResult {
    bool ok = false;
    bool usedSdk = false;
    QString lua, error, out, err;
    QStringList log, warnings, files;
    QMap<QString, QList<QVariantMap>> addItems;
    // luanti_creator_core : le script a importé le module (coreUsed) et demande ces modifications de LuantiCreator.
    // Elles ne sont appliquées par PluginHost que si l'utilisateur a autorisé le plugin à modifier LuantiCreator.
    bool coreUsed = false;
    QJsonArray core;
};

namespace lcscript {

constexpr int kTimeoutMs = 30000;
constexpr qint64 kMaxOutput = 4 * 1024 * 1024;

// interpréteurs possibles par extension (le premier trouvé dans le PATH est utilisé)
inline QStringList candidates(const QString &ext)
{
    if (ext == "py") return {"python3", "python"};
    if (ext == "sh") return {"sh", "bash"};
    if (ext == "js") return {"node", "nodejs"};
    if (ext == "rb") return {"ruby"};
    if (ext == "lua") return {"lua5.4", "lua5.3", "lua", "luajit"};
    if (ext == "pl") return {"perl"};
    return {};
}

inline bool supportedExt(const QString &ext) { return !candidates(ext).isEmpty(); }

// `requested` : interpréteur demandé par le manifeste (vide = d'après l'extension) ; seuls ceux de la liste sont acceptés
inline QString findInterpreter(const QString &file, const QString &requested, QString &err)
{
    const QString ext = QFileInfo(file).suffix().toLower();
    QStringList cands = candidates(ext);
    if (cands.isEmpty()) {
        err = QObject::tr("type de script non pris en charge : .%1 (py, sh, js, rb, lua, pl)").arg(ext);
        return QString();
    }
    if (!requested.isEmpty()) {
        QStringList allowed;
        for (const char *e : {"py", "sh", "js", "rb", "lua", "pl"}) allowed << candidates(e);
        allowed << "bash";
        if (!allowed.contains(requested)) {
            err = QObject::tr("interpréteur « %1 » non autorisé (autorisés : %2)").arg(requested, allowed.join(", "));
            return QString();
        }
        cands = QStringList{requested};
    }
    for (const QString &c : cands) {
        const QString exe = QStandardPaths::findExecutable(c);
        if (!exe.isEmpty()) return exe;
    }
    err = QObject::tr("interpréteur introuvable : installez « %1 »").arg(cands.first());
    return QString();
}

inline bool safeRel(const QString &rel)
{
    if (rel.isEmpty() || rel.startsWith('/') || rel.contains('\\') || rel.contains(':')) return false;
    for (const QString &seg : rel.split('/'))
        if (seg == ".." || seg.isEmpty()) return false;
    return true;
}

// fichiers qu'un script peut ajouter au mod
inline bool outputFileAllowed(const QString &rel)
{
    if (!safeRel(rel)) return false;
    if (rel == "init.lua" || rel == "mod.conf") return false; // générés par LuantiCreator
    static const QStringList ok = {"png", "jpg", "jpeg", "ogg", "obj", "b3d", "x", "lua", "txt", "md", "json", "conf", "tr"};
    return ok.contains(QFileInfo(rel).suffix().toLower());
}

// Lance le script. `modRoot` : dossier du mod où copier les fichiers produits (vide = on ne copie rien).
inline ScriptResult run(const QString &pluginDir, const QString &scriptRel, const QString &interpreter,
                        QJsonObject ctx, const QString &modRoot, int timeoutMs = kTimeoutMs)
{
    ScriptResult r;
    if (!safeRel(scriptRel)) {
        r.error = QObject::tr("chemin de script non autorisé : %1").arg(scriptRel);
        return r;
    }
    const QString scriptAbs = QDir(pluginDir).absoluteFilePath(scriptRel);
    if (!QFile::exists(scriptAbs)) {
        r.error = QObject::tr("script introuvable : %1").arg(scriptRel);
        return r;
    }
    QString ierr;
    const QString exe = findInterpreter(scriptRel, interpreter, ierr);
    if (exe.isEmpty()) {
        r.error = ierr;
        return r;
    }
    QTemporaryDir tmp;
    if (!tmp.isValid()) {
        r.error = QObject::tr("dossier temporaire indisponible");
        return r;
    }
    const QString sdkDir = tmp.path() + "/sdk", outDir = tmp.path() + "/out";
    QDir().mkpath(sdkDir);
    QDir().mkpath(outDir);
    {
        QFile f(sdkDir + "/luanti_creator.py");
        if (f.open(QIODevice::WriteOnly)) f.write(lcsdk::python());
    }
    {
        QFile f(sdkDir + "/luanti_creator_core.py");
        if (f.open(QIODevice::WriteOnly)) f.write(lcsdk::pythonCore());
    }
    ctx["api"] = 1;
    {
        QFile f(tmp.path() + "/context.json");
        if (!f.open(QIODevice::WriteOnly)) {
            r.error = QObject::tr("impossible d'écrire le contexte");
            return r;
        }
        f.write(QJsonDocument(ctx).toJson(QJsonDocument::Compact));
    }
    const QString resultPath = tmp.path() + "/result.json";

    QProcess proc;
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("LC_CONTEXT", tmp.path() + "/context.json");
    env.insert("LC_RESULT", resultPath);
    env.insert("LC_OUT_DIR", outDir);
    env.insert("LC_SDK_DIR", sdkDir);
    env.insert("LC_PLUGIN_DIR", pluginDir);
    env.insert("LC_MOD_DIR", modRoot);
    env.insert("PYTHONPATH", env.contains("PYTHONPATH") ? sdkDir + ":" + env.value("PYTHONPATH") : sdkDir);
    env.insert("PYTHONIOENCODING", "utf-8");
    env.insert("PYTHONDONTWRITEBYTECODE", "1");
    env.insert("NODE_PATH", sdkDir);
    proc.setProcessEnvironment(env);
    proc.setWorkingDirectory(pluginDir);
    proc.start(exe, {scriptAbs});
    if (!proc.waitForStarted(5000)) {
        r.error = QObject::tr("impossible de lancer %1").arg(exe);
        return r;
    }
    if (!proc.waitForFinished(timeoutMs)) {
        proc.kill();
        proc.waitForFinished(2000);
        r.error = QObject::tr("script arrêté : plus de %1 s d'exécution").arg(timeoutMs / 1000);
        return r;
    }
    r.out = QString::fromUtf8(proc.readAllStandardOutput().left(kMaxOutput));
    r.err = QString::fromUtf8(proc.readAllStandardError().left(kMaxOutput));

    QStringList errors;
    QFile rf(resultPath);
    if (rf.open(QIODevice::ReadOnly)) {
        const QJsonObject o = QJsonDocument::fromJson(rf.readAll()).object();
        r.usedSdk = !o.isEmpty();
        r.lua = o["lua"].toString();
        for (const QJsonValue &v : o["log"].toArray()) r.log << v.toString();
        for (const QJsonValue &v : o["warnings"].toArray()) r.warnings << v.toString();
        for (const QJsonValue &v : o["errors"].toArray()) errors << v.toString();
        r.coreUsed = o["core_used"].toBool();
        r.core = o["core"].toArray();
        const QJsonObject ai = o["add_items"].toObject();
        for (auto it = ai.begin(); it != ai.end(); ++it) {
            QList<QVariantMap> l;
            for (const QJsonValue &v : it.value().toArray())
                if (v.isObject()) l << v.toObject().toVariantMap();
            if (!l.isEmpty()) r.addItems[it.key()] = l;
        }
    } else {
        r.lua = r.out; // script sans bibliothèque : la sortie standard est le code Lua
    }

    // fichiers produits -> dossier du mod
    int count = 0;
    QDirIterator di(outDir, QDir::Files, QDirIterator::Subdirectories);
    while (di.hasNext()) {
        const QString abs = di.next();
        const QString rel = QDir(outDir).relativeFilePath(abs);
        if (!outputFileAllowed(rel)) {
            r.warnings << QObject::tr("fichier « %1 » ignoré (nom ou type non autorisé)").arg(rel);
            continue;
        }
        if (++count > 300 || QFileInfo(abs).size() > 16 * 1024 * 1024) {
            r.warnings << QObject::tr("fichier « %1 » ignoré (trop de fichiers ou fichier trop gros)").arg(rel);
            continue;
        }
        if (modRoot.isEmpty()) continue;
        const QString dst = modRoot + "/" + rel;
        QDir().mkpath(QFileInfo(dst).absolutePath());
        QFile::remove(dst);
        if (QFile::copy(abs, dst)) r.files << rel;
        else r.warnings << QObject::tr("impossible d'écrire %1").arg(rel);
    }

    if (proc.exitStatus() != QProcess::NormalExit || proc.exitCode() != 0) {
        QString e = r.err.trimmed();
        if (e.size() > 800) e = "…" + e.right(800);
        errors << QObject::tr("le script s'est terminé avec une erreur (code %1)%2")
                      .arg(proc.exitCode()).arg(e.isEmpty() ? QString() : " :\n" + e);
    }
    r.error = errors.join("\n");
    r.ok = errors.isEmpty();
    return r;
}

} // namespace lcscript

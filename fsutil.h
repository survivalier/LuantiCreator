#pragma once
// Utilitaires fichiers : copie récursive, écriture d'archives .zip (sans dépendance),
// détection des dossiers de mods de Luanti.

#include <QtCore>
#include <string>
#include "zipread.h"

inline bool copyDir(const QString &src, const QString &dst, QString *err = nullptr)
{
    const QDir s(src);
    if (!s.exists()) {
        if (err) *err = QObject::tr("Dossier source introuvable : %1").arg(src);
        return false;
    }
    if (!QDir().mkpath(dst)) {
        if (err) *err = QObject::tr("Impossible de créer le dossier : %1").arg(dst);
        return false;
    }
    const QFileInfoList entries =
        s.entryInfoList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden);
    for (const QFileInfo &fi : entries) {
        const QString to = dst + "/" + fi.fileName();
        if (fi.isDir()) {
            if (!copyDir(fi.absoluteFilePath(), to, err)) return false;
        } else {
            QFile::remove(to);
            if (!QFile::copy(fi.absoluteFilePath(), to)) {
                if (err) *err = QObject::tr("Impossible de copier %1 vers %2").arg(fi.absoluteFilePath(), to);
                return false;
            }
        }
    }
    return true;
}

inline quint32 crc32Of(const QByteArray &d)
{
    static quint32 table[256];
    static bool ready = false;
    if (!ready) {
        for (quint32 i = 0; i < 256; ++i) {
            quint32 c = i;
            for (int k = 0; k < 8; ++k) c = (c & 1) ? (0xEDB88320u ^ (c >> 1)) : (c >> 1);
            table[i] = c;
        }
        ready = true;
    }
    quint32 c = 0xFFFFFFFFu;
    for (char b : d) c = table[(c ^ quint8(b)) & 0xFF] ^ (c >> 8);
    return c ^ 0xFFFFFFFFu;
}

// Crée une archive .zip contenant tous les fichiers de rootDir, rangés sous « prefix/ ».
// (format identique à celui des mods de ContentDB : un dossier racine portant le nom du mod)
inline bool writeZip(const QString &zipPath, const QString &rootDir, const QString &prefix,
                     QString *err = nullptr)
{
    QFile out(zipPath);
    if (!out.open(QIODevice::WriteOnly)) {
        if (err) *err = QObject::tr("Impossible d'écrire %1").arg(zipPath);
        return false;
    }
    struct Entry {
        QByteArray name;
        quint32 crc = 0, csize = 0, usize = 0, offset = 0;
        quint16 method = 0;
    };
    QVector<Entry> entries;
    const QDir root(rootDir);

    QStringList files;
    QDirIterator it(rootDir, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (it.hasNext()) files << it.next();
    files.sort();

    const QDateTime now = QDateTime::currentDateTime();
    const quint16 dosTime = quint16((now.time().hour() << 11) | (now.time().minute() << 5) | (now.time().second() / 2));
    const quint16 dosDate = quint16(((now.date().year() - 1980) << 9) | (now.date().month() << 5) | now.date().day());

    auto w16 = [&out](quint16 v) {
        const char b[2] = {char(v & 0xFF), char((v >> 8) & 0xFF)};
        out.write(b, 2);
    };
    auto w32 = [&out](quint32 v) {
        const char b[4] = {char(v & 0xFF), char((v >> 8) & 0xFF), char((v >> 16) & 0xFF), char((v >> 24) & 0xFF)};
        out.write(b, 4);
    };

    for (const QString &path : files) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) {
            if (err) *err = QObject::tr("Impossible de lire %1").arg(path);
            out.close();
            QFile::remove(zipPath);
            return false;
        }
        const QByteArray data = f.readAll();
        Entry e;
        e.name = (prefix.isEmpty() ? QString() : prefix + "/").toUtf8() + root.relativeFilePath(path).toUtf8();
        e.crc = crc32Of(data);
        e.usize = quint32(data.size());
        e.offset = quint32(out.pos());

        QByteArray payload = data;
        if (data.size() > 64) {
            // qCompress = 4 octets de taille + flux zlib (2 octets d'en-tête, deflate, 4 octets Adler-32)
            const QByteArray z = qCompress(data, 9);
            if (z.size() > 10) {
                const QByteArray raw = z.mid(6, z.size() - 10);
                if (raw.size() < data.size()) {
                    payload = raw;
                    e.method = 8;
                }
            }
        }
        e.csize = quint32(payload.size());

        w32(0x04034b50);
        w16(20);
        w16(0x0800); // noms en UTF-8
        w16(e.method);
        w16(dosTime);
        w16(dosDate);
        w32(e.crc);
        w32(e.csize);
        w32(e.usize);
        w16(quint16(e.name.size()));
        w16(0);
        out.write(e.name);
        out.write(payload);
        entries.append(e);
    }

    const quint32 cdStart = quint32(out.pos());
    for (const Entry &e : entries) {
        w32(0x02014b50);
        w16(0x031E); // créé sous Unix
        w16(20);
        w16(0x0800);
        w16(e.method);
        w16(dosTime);
        w16(dosDate);
        w32(e.crc);
        w32(e.csize);
        w32(e.usize);
        w16(quint16(e.name.size()));
        w16(0);
        w16(0);
        w16(0);
        w16(0);
        w32(0x81A40000u); // -rw-r--r--
        w32(e.offset);
        out.write(e.name);
    }
    const quint32 cdSize = quint32(out.pos()) - cdStart;
    w32(0x06054b50);
    w16(0);
    w16(0);
    w16(quint16(entries.size()));
    w16(quint16(entries.size()));
    w32(cdSize);
    w32(cdStart);
    w16(0);
    out.close();
    return true;
}

// Dossiers « mods » de Luanti existants sur cette machine (ou le dossier par défaut).
inline QStringList luantiModDirs()
{
    const QString h = QDir::homePath();
    const QStringList cand = {h + "/.minetest/mods",
                              h + "/.luanti/mods",
                              h + "/.local/share/luanti/mods",
                              h + "/.local/share/minetest/mods",
                              h + "/.var/app/org.luanti.luanti/.minetest/mods",
                              h + "/.var/app/org.luanti.luanti/.luanti/mods",
                              h + "/.var/app/net.minetest.Minetest/.minetest/mods"};
    QStringList out;
    for (const QString &c : cand)
        if (QDir(c).exists()) out << c;
    if (out.isEmpty()) out << h + "/.minetest/mods";
    return out;
}

// ------------------------------------------------------- mods contenus dans un .zip

struct ZipModInfo {
    QString prefix;       // dossier du mod dans l'archive ("" = racine)
    QString name;         // nom du mod = dossier d'installation
    QString title;
    QStringList depends;  // dépendances obligatoires
};

inline bool loadZipBytes(const QString &zipPath, std::string &bytes, QString *err)
{
    QFile f(zipPath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (err) *err = QObject::tr("Impossible d'ouvrir %1").arg(zipPath);
        return false;
    }
    if (f.size() > 300LL * 1024 * 1024) {
        if (err) *err = QObject::tr("Archive trop volumineuse (plus de 300 Mo).");
        return false;
    }
    const QByteArray b = f.readAll();
    bytes.assign(b.constData(), size_t(b.size()));
    return true;
}

inline bool loadZipFiles(const QString &zipPath, std::string &z, std::vector<lczip::Entry> &files, QString *err)
{
    if (!loadZipBytes(zipPath, z, err)) return false;
    std::vector<lczip::Entry> all;
    std::string e;
    if (!lczip::listEntries(z, all, e) || !lczip::filterFiles(all, files, e)) {
        if (err) *err = QString::fromUtf8(e.c_str());
        return false;
    }
    return true;
}

inline QString modNameFromFolder(const std::string &prefix, const QString &fallbackBase)
{
    std::string base = prefix;
    while (!base.empty() && base.back() == '/') base.pop_back();
    const size_t sl = base.rfind('/');
    if (sl != std::string::npos) base = base.substr(sl + 1);
    QString n = QString::fromStdString(lczip::sanitizeName(base));
    if (n.isEmpty()) n = QString::fromStdString(lczip::sanitizeName(fallbackBase.toStdString()));
    return n.isEmpty() ? QStringLiteral("mod") : n;
}

// Liste les mods Luanti présents dans l'archive (un mod, ou plusieurs pour un modpack).
inline bool zipListMods(const QString &zipPath, QVector<ZipModInfo> &mods, QString *err)
{
    std::string z;
    std::vector<lczip::Entry> files;
    if (!loadZipFiles(zipPath, z, files, err)) return false;
    const std::vector<std::string> roots = lczip::findModRoots(files);
    if (roots.empty()) {
        if (err) *err = QObject::tr("Aucun mod Luanti dans cette archive (ni mod.conf, ni init.lua).");
        return false;
    }
    QSet<QString> used;
    for (const std::string &r : roots) {
        std::string conf, dep, e;
        for (const lczip::Entry &en : files) {
            if (en.name == r + "mod.conf") {
                if (!lczip::readEntry(z, en, conf, e)) {
                    if (err) *err = QString::fromUtf8(e.c_str());
                    return false;
                }
            } else if (en.name == r + "depends.txt") {
                lczip::readEntry(z, en, dep, e); // facultatif
            }
        }
        const lczip::ModMeta m = lczip::parseMeta(conf, dep);
        QString name = QString::fromStdString(lczip::sanitizeName(m.name));
        if (name.isEmpty()) name = modNameFromFolder(r, QFileInfo(zipPath).completeBaseName());
        QString unique = name;
        for (int k = 2; used.contains(unique); ++k) unique = name + "_" + QString::number(k);
        used.insert(unique);

        ZipModInfo info;
        info.prefix = QString::fromStdString(r);
        info.name = unique;
        info.title = QString::fromStdString(m.title);
        for (const std::string &d : m.depends) info.depends << QString::fromStdString(d);
        mods.append(info);
    }
    return true;
}

// Extrait les mods choisis dans destRoot/<nom>/ (le dossier existant est remplacé).
inline bool zipExtractMods(const QString &zipPath, const QVector<ZipModInfo> &mods, const QString &destRoot,
                           QString *err)
{
    std::string z;
    std::vector<lczip::Entry> files;
    if (!loadZipFiles(zipPath, z, files, err)) return false;
    quint64 total = 0;
    for (const ZipModInfo &m : mods) {
        const std::string prefix = m.prefix.toStdString();
        const QString base = QDir::cleanPath(destRoot + "/" + m.name);
        QDir(base).removeRecursively();
        if (!QDir().mkpath(base)) {
            if (err) *err = QObject::tr("Impossible de créer %1").arg(base);
            return false;
        }
        for (const lczip::Entry &en : files) {
            if (en.name.compare(0, prefix.size(), prefix) != 0) continue;
            const std::string rel = en.name.substr(prefix.size());
            if (rel.empty()) continue;
            std::string data, e;
            if (!lczip::readEntry(z, en, data, e)) {
                if (err) *err = QString::fromUtf8(e.c_str());
                QDir(base).removeRecursively();
                return false;
            }
            total += data.size();
            if (total > 400ull * 1024 * 1024) {
                if (err) *err = QObject::tr("Archive trop volumineuse une fois décompressée.");
                QDir(base).removeRecursively();
                return false;
            }
            const QString path = QDir::cleanPath(base + "/" + QString::fromStdString(rel));
            if (!path.startsWith(base + "/")) { // garde-fou supplémentaire contre les chemins piégés
                if (err) *err = QObject::tr("Chemin dangereux dans l'archive : %1").arg(QString::fromStdString(rel));
                QDir(base).removeRecursively();
                return false;
            }
            QDir().mkpath(QFileInfo(path).absolutePath());
            QFile out(path);
            if (!out.open(QIODevice::WriteOnly) || out.write(data.data(), qint64(data.size())) != qint64(data.size())) {
                if (err) *err = QObject::tr("Impossible d'écrire %1").arg(path);
                return false;
            }
        }
    }
    return true;
}

inline std::string readTextFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return std::string();
    const QByteArray b = f.readAll();
    return std::string(b.constData(), size_t(b.size()));
}

// Même détection pour un dossier de mod déjà décompressé.
inline bool inspectModDir(const QString &dir, ZipModInfo &info, QString *err)
{
    const QDir d(dir);
    if (!d.exists("mod.conf") && !d.exists("init.lua")) {
        if (err) *err = QObject::tr("Ce dossier ne contient pas de mod Luanti (ni mod.conf, ni init.lua).");
        return false;
    }
    const lczip::ModMeta m = lczip::parseMeta(readTextFile(dir + "/mod.conf"), readTextFile(dir + "/depends.txt"));
    QString name = QString::fromStdString(lczip::sanitizeName(m.name));
    if (name.isEmpty()) name = modNameFromFolder(std::string(), QFileInfo(dir).fileName());
    info.prefix.clear();
    info.name = name;
    info.title = QString::fromStdString(m.title);
    info.depends.clear();
    for (const std::string &x : m.depends) info.depends << QString::fromStdString(x);
    return true;
}

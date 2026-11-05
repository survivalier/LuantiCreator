#pragma once
// Chargement de modèles 3D (OBJ complet, glTF/GLB boîte englobante) et
// génération automatique de selectionbox / collision box pour Luanti.

#include <QtCore>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVector3D>
#include <array>
#include <cmath>

using Box6 = std::array<double, 6>; // x1,y1,z1,x2,y2,z2

struct Mesh {
    QVector<QVector3D> v;
    QVector<std::array<int, 3>> tris;
    QVector3D mn{0, 0, 0}, mx{0, 0, 0};
    bool valid = false;        // boîte englobante connue
    bool hasGeometry = false;  // triangles disponibles
    QString error;
};

inline Box6 mkbox(double a, double b, double c, double d, double e, double f)
{
    return Box6{{a, b, c, d, e, f}};
}

inline QString numStr(double v)
{
    QString s = QString::number(v, 'f', 4);
    if (s.contains('.')) {
        while (s.endsWith('0')) s.chop(1);
        if (s.endsWith('.')) s.chop(1);
    }
    if (s == "-0") s = "0";
    return s;
}

inline void meshComputeBBox(Mesh &m)
{
    if (m.v.isEmpty()) return;
    m.mn = m.mx = m.v[0];
    for (const QVector3D &p : m.v) {
        m.mn.setX(qMin(m.mn.x(), p.x())); m.mx.setX(qMax(m.mx.x(), p.x()));
        m.mn.setY(qMin(m.mn.y(), p.y())); m.mx.setY(qMax(m.mx.y(), p.y()));
        m.mn.setZ(qMin(m.mn.z(), p.z())); m.mx.setZ(qMax(m.mx.z(), p.z()));
    }
    m.valid = true;
}

inline Mesh loadObj(const QString &path)
{
    Mesh m;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        m.error = QObject::tr("Impossible d'ouvrir le fichier.");
        return m;
    }
    QTextStream in(&f);
    const QRegularExpression ws("\\s+");
    QVector<QVector<int>> faces;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.startsWith("v ")) {
            const QStringList p = line.split(ws, Qt::SkipEmptyParts);
            if (p.size() >= 4)
                m.v.append(QVector3D(p[1].toFloat(), p[2].toFloat(), p[3].toFloat()));
        } else if (line.startsWith("f ")) {
            const QStringList p = line.split(ws, Qt::SkipEmptyParts);
            QVector<int> idx;
            for (int i = 1; i < p.size(); ++i) {
                int vi = p[i].section('/', 0, 0).toInt();
                if (vi < 0) vi = m.v.size() + vi;
                else vi -= 1;
                idx.append(vi);
            }
            faces.append(idx);
        }
    }
    for (const QVector<int> &idx : faces) {
        for (int i = 1; i + 1 < idx.size(); ++i) {
            const int a = idx[0], b = idx[i], c = idx[i + 1];
            if (a >= 0 && b >= 0 && c >= 0 && a < m.v.size() && b < m.v.size() && c < m.v.size())
                m.tris.push_back(std::array<int, 3>{{a, b, c}});
        }
    }
    meshComputeBBox(m);
    m.hasGeometry = m.valid && !m.tris.isEmpty();
    if (!m.valid) m.error = QObject::tr("Aucun sommet trouvé dans le fichier OBJ.");
    return m;
}

// glTF / GLB : on lit uniquement les min/max des accessors POSITION.
inline Mesh loadGltf(const QString &path)
{
    Mesh m;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        m.error = QObject::tr("Impossible d'ouvrir le fichier.");
        return m;
    }
    const QByteArray data = f.readAll();
    QByteArray json = data;
    if (data.startsWith("glTF")) {
        if (data.size() < 20) {
            m.error = QObject::tr("Fichier GLB invalide.");
            return m;
        }
        const quint32 len = qFromLittleEndian<quint32>(
            reinterpret_cast<const uchar *>(data.constData() + 12));
        json = data.mid(20, int(len));
    }
    const QJsonObject o = QJsonDocument::fromJson(json).object();
    const QJsonArray accessors = o["accessors"].toArray();
    bool any = false;
    QVector3D mn, mx;
    for (const QJsonValue &mv : o["meshes"].toArray()) {
        for (const QJsonValue &pv : mv.toObject()["primitives"].toArray()) {
            const int acc = pv.toObject()["attributes"].toObject()["POSITION"].toInt(-1);
            if (acc < 0 || acc >= accessors.size()) continue;
            const QJsonObject a = accessors[acc].toObject();
            const QJsonArray amin = a["min"].toArray(), amax = a["max"].toArray();
            if (amin.size() < 3 || amax.size() < 3) continue;
            const QVector3D lo(amin[0].toDouble(), amin[1].toDouble(), amin[2].toDouble());
            const QVector3D hi(amax[0].toDouble(), amax[1].toDouble(), amax[2].toDouble());
            if (!any) { mn = lo; mx = hi; any = true; }
            else {
                mn.setX(qMin(mn.x(), lo.x())); mn.setY(qMin(mn.y(), lo.y())); mn.setZ(qMin(mn.z(), lo.z()));
                mx.setX(qMax(mx.x(), hi.x())); mx.setY(qMax(mx.y(), hi.y())); mx.setZ(qMax(mx.z(), hi.z()));
            }
        }
    }
    if (!any) {
        m.error = QObject::tr("Impossible de déterminer la taille du modèle glTF.");
        return m;
    }
    m.mn = mn; m.mx = mx; m.valid = true; m.hasGeometry = false;
    return m;
}

inline Mesh loadModel(const QString &path)
{
    Mesh m;
    if (path.isEmpty() || !QFile::exists(path)) {
        m.error = QObject::tr("Aucun fichier de modèle.");
        return m;
    }
    const QString ext = QFileInfo(path).suffix().toLower();
    if (ext == "obj") return loadObj(path);
    if (ext == "gltf" || ext == "glb") return loadGltf(path);
    m.error = QObject::tr("Format .%1 : pas d'aperçu ni de calcul automatique (utilisez le mode manuel).").arg(ext);
    return m;
}

using MeshPtr = QSharedPointer<const Mesh>;

// Charge un modèle avec cache (clé : chemin + date de modification).
inline MeshPtr cachedMesh(const QString &path, bool force = false)
{
    static QHash<QString, QPair<qint64, MeshPtr>> cache;
    const qint64 mt = QFileInfo(path).lastModified().toMSecsSinceEpoch();
    auto it = cache.find(path);
    if (!force && it != cache.end() && it->first == mt) return it->second;
    MeshPtr p(new Mesh(loadModel(path)));
    cache.insert(path, qMakePair(mt, p));
    return p;
}

// Cache : ne recharge le modèle que si le fichier ou sa date changent.
struct MeshCache {
    Mesh mesh;
    QString path;
    qint64 mt = 0;
    const Mesh &get(const QString &p)
    {
        const qint64 t = QFileInfo(p).lastModified().toMSecsSinceEpoch();
        if (p != path || t != mt) {
            mesh = loadModel(p);
            path = p;
            mt = t;
        }
        return mesh;
    }
};

// ------------------------------------------------------------- boîtes

inline QVector<Box6> parseBoxes(const QString &text)
{
    QVector<Box6> r;
    const QRegularExpression sep("[\\s,;]+");
    for (const QString &line : text.split('\n')) {
        const QStringList p = line.trimmed().split(sep, Qt::SkipEmptyParts);
        if (p.size() < 6) continue;
        Box6 b;
        bool ok = true;
        for (int i = 0; i < 6 && ok; ++i) b[i] = p[i].toDouble(&ok);
        if (!ok) continue;
        for (int i = 0; i < 3; ++i)
            if (b[i] > b[i + 3]) std::swap(b[i], b[i + 3]);
        r.append(b);
    }
    return r;
}

inline QString formatBoxes(const QVector<Box6> &boxes)
{
    QStringList lines;
    for (const Box6 &b : boxes) {
        QStringList n;
        for (double v : b) n << numStr(v);
        lines << n.join(' ');
    }
    return lines.join('\n');
}

// Modes : 0 AABB, 1 AABB réduite, 2 voxels (multi-boîtes), 3 cube complet, 4 manuel
// Les coordonnées renvoyées sont en unités de node (le modèle est divisé par `scale`).
inline QVector<Box6> generateBoxes(const Mesh &m, int mode, double scale, double pad,
                                   int grid, int maxBoxes)
{
    QVector<Box6> out;
    if (mode == 3) {
        out.append(mkbox(-0.5, -0.5, -0.5, 0.5, 0.5, 0.5));
        return out;
    }
    if (!m.valid || mode == 4) return out;
    if (scale <= 1e-6) scale = 1.0;

    double x1 = m.mn.x() / scale, y1 = m.mn.y() / scale, z1 = m.mn.z() / scale;
    double x2 = m.mx.x() / scale, y2 = m.mx.y() / scale, z2 = m.mx.z() / scale;
    const double ex = x2 - x1, ey = y2 - y1, ez = z2 - z1;
    const double maxExt = qMax(ex, qMax(ey, ez));

    if (mode == 0 || (mode == 2 && (!m.hasGeometry || maxExt < 1e-6))) {
        out.append(mkbox(x1, y1, z1, x2, y2, z2));
        return out;
    }
    if (mode == 1) {
        auto shrink = [&](double &lo, double &hi) {
            if (hi - lo > 2 * pad + 0.02) { lo += pad; hi -= pad; }
        };
        shrink(x1, x2); shrink(y1, y2); shrink(z1, z2);
        out.append(mkbox(x1, y1, z1, x2, y2, z2));
        return out;
    }

    // --- mode 2 : voxelisation puis fusion gloutonne en boîtes
    int N = qBound(2, grid, 64);
    maxBoxes = qMax(1, maxBoxes);
    for (;;) {
        const double cell = maxExt / N;
        const int nx = qMax(1, int(std::ceil(ex / cell - 1e-6)));
        const int ny = qMax(1, int(std::ceil(ey / cell - 1e-6)));
        const int nz = qMax(1, int(std::ceil(ez / cell - 1e-6)));
        QVector<char> occ(nx * ny * nz, 0);
        auto mark = [&](double px, double py, double pz) {
            const int ix = qBound(0, int(std::floor((px - x1) / cell)), nx - 1);
            const int iy = qBound(0, int(std::floor((py - y1) / cell)), ny - 1);
            const int iz = qBound(0, int(std::floor((pz - z1) / cell)), nz - 1);
            occ[(iz * ny + iy) * nx + ix] = 1;
        };
        for (const auto &t : m.tris) {
            const QVector3D a = m.v[t[0]] / float(scale);
            const QVector3D b = m.v[t[1]] / float(scale);
            const QVector3D c = m.v[t[2]] / float(scale);
            const float L = qMax(qMax((b - a).length(), (c - b).length()), (a - c).length());
            const int n = qBound(1, int(std::ceil(L / (cell * 0.5))), 120);
            for (int i = 0; i <= n; ++i)
                for (int j = 0; j <= n - i; ++j) {
                    const float u = float(i) / n, w = float(j) / n;
                    const QVector3D p = a * (1 - u - w) + b * u + c * w;
                    mark(p.x(), p.y(), p.z());
                }
        }

        QVector<char> used(occ.size(), 0);
        auto idx = [&](int x, int y, int z) { return (z * ny + y) * nx + x; };
        auto freeCell = [&](int x, int y, int z) {
            const int i = idx(x, y, z);
            return occ[i] && !used[i];
        };
        QVector<Box6> res;
        for (int z = 0; z < nz; ++z)
            for (int y = 0; y < ny; ++y)
                for (int x = 0; x < nx; ++x) {
                    if (!freeCell(x, y, z)) continue;
                    int xe = x;
                    while (xe + 1 < nx && freeCell(xe + 1, y, z)) ++xe;
                    int ye = y;
                    for (;;) {
                        if (ye + 1 >= ny) break;
                        bool ok = true;
                        for (int xx = x; xx <= xe && ok; ++xx) ok = freeCell(xx, ye + 1, z);
                        if (!ok) break;
                        ++ye;
                    }
                    int ze = z;
                    for (;;) {
                        if (ze + 1 >= nz) break;
                        bool ok = true;
                        for (int yy = y; yy <= ye && ok; ++yy)
                            for (int xx = x; xx <= xe && ok; ++xx) ok = freeCell(xx, yy, ze + 1);
                        if (!ok) break;
                        ++ze;
                    }
                    for (int zz = z; zz <= ze; ++zz)
                        for (int yy = y; yy <= ye; ++yy)
                            for (int xx = x; xx <= xe; ++xx) used[idx(xx, yy, zz)] = 1;
                    res.append(mkbox(x1 + x * cell, y1 + y * cell, z1 + z * cell,
                                     qMin(x2, x1 + (xe + 1) * cell),
                                     qMin(y2, y1 + (ye + 1) * cell),
                                     qMin(z2, z1 + (ze + 1) * cell)));
                }
        if (res.size() <= maxBoxes || N <= 2) {
            out = res;
            break;
        }
        N = qMax(2, N * 3 / 4);
    }
    return out;
}

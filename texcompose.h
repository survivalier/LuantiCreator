#pragma once
// Textures animées et textures superposées.
//
// • Animation : une texture animée de Luanti est une image verticale (« bande ») dont chaque image est un carré,
//   ou une « planche » en grille. Le mode « Auto » reconnaît une bande (hauteur = N × largeur).
// • Superposition : jusqu'à deux textures posées par-dessus la texture de base (transparence respectée,
//   opacité réglable). Elles sont fusionnées dans une seule image PNG à la génération du mod, ce qui marche aussi
//   avec une base animée (la texture par-dessus est alors répétée sur chaque image de l'animation).
//
// Clés lues dans les données d'un élément (toutes facultatives, un ancien projet reste valide) :
//   overlay, overlay2          chemins des textures par-dessus
//   overlay_opacity            0-100 (défaut 100)
//   anim                       « Auto… » (défaut) · « Aucune » · « Bande verticale » · « Planche… »
//   anim_x, anim_y             images de la bande (anim_x) ou colonnes × lignes de la planche
//   anim_time                  durée totale d'un cycle d'animation, en secondes

#include <QtCore>
#include <QtGui>

namespace lctx {

struct Anim {
    enum Type { None, Vertical, Sheet } type = None;
    int cols = 1, rows = 1;  // bande verticale : cols = 1, rows = nombre d'images
    double time = 1.0;       // durée totale d'un cycle (s)
    int frames() const { return qMax(1, cols * rows); }
    double frameSeconds() const { return time / frames(); }
};

enum class Mode { Auto, None, Vertical, Sheet };

inline const QStringList &modeLabels()
{
    static const QStringList l = {QStringLiteral("Auto (bande verticale détectée)"), QStringLiteral("Aucune"),
                                  QStringLiteral("Bande verticale"), QStringLiteral("Planche (colonnes × lignes)")};
    return l;
}

inline Mode modeOf(const QVariantMap &m)
{
    const QString s = m.value("anim").toString().trimmed().toLower();
    if (s.startsWith("aucun")) return Mode::None;
    if (s.startsWith("bande")) return Mode::Vertical;
    if (s.startsWith("planche")) return Mode::Sheet;
    return Mode::Auto; // absent ou « Auto… »
}

// Réglage d'animation pour cette image de base. Type None si l'animation est désactivée,
// si l'image n'en est pas une, ou si les dimensions ne tombent pas juste.
inline Anim animFor(const QVariantMap &m, const QImage &base)
{
    Anim a;
    const double t = m.value("anim_time", 1.0).toDouble();
    a.time = t > 0.01 ? t : 1.0;
    const int w = base.width(), h = base.height();
    if (w <= 0 || h <= 0) return a;
    switch (modeOf(m)) {
    case Mode::None: break;
    case Mode::Auto:
        if (h > w && h % w == 0 && h / w <= 4096) {
            a.type = Anim::Vertical;
            a.rows = h / w;
        }
        break;
    case Mode::Vertical: {
        const int n = m.value("anim_x", 1).toInt();
        if (n > 1 && n <= 4096 && h % n == 0) {
            a.type = Anim::Vertical;
            a.rows = n;
        }
        break;
    }
    case Mode::Sheet: {
        const int c = qMax(1, m.value("anim_x", 1).toInt()), r = qMax(1, m.value("anim_y", 1).toInt());
        if (c * r > 1 && c <= 256 && r <= 256 && w % c == 0 && h % r == 0) {
            a.type = Anim::Sheet;
            a.cols = c;
            a.rows = r;
        }
        break;
    }
    }
    return a;
}

inline QSize frameSize(const QImage &img, const Anim &a)
{
    return QSize(img.width() / qMax(1, a.cols), img.height() / qMax(1, a.rows));
}

// image n° idx (0 = première) d'une texture animée
inline QImage frame(const QImage &img, const Anim &a, int idx)
{
    if (a.type == Anim::None || img.isNull()) return img;
    const QSize fs = frameSize(img, a);
    idx = ((idx % a.frames()) + a.frames()) % a.frames();
    const int cx = idx % a.cols, cy = idx / a.cols;
    return img.copy(cx * fs.width(), cy * fs.height(), fs.width(), fs.height());
}

// image à afficher maintenant (selon l'horloge)
inline int currentFrame(const Anim &a)
{
    if (a.frames() <= 1) return 0;
    const qint64 ms = QDateTime::currentMSecsSinceEpoch();
    const qint64 step = qMax<qint64>(30, qint64(a.frameSeconds() * 1000.0));
    return int((ms / step) % a.frames());
}

inline int opacityOf(const QVariantMap &m)
{
    return m.contains("overlay_opacity") ? qBound(0, m.value("overlay_opacity").toInt(), 100) : 100;
}

// Dessine les textures par-dessus la base. Une texture par-dessus qui a la proportion d'UNE image de l'animation
// est répétée sur chaque image ; sinon elle est étirée sur toute l'image de base (par exemple une bande de même taille).
inline QImage compose(const QImage &base, const QList<QImage> &overlays, int opacityPct, const Anim &anim)
{
    QImage out = base.convertToFormat(QImage::Format_ARGB32);
    if (out.isNull()) return out;
    const double rb = double(out.width()) / out.height();
    const QSize fs = frameSize(out, anim);
    const double rf = fs.height() > 0 ? double(fs.width()) / fs.height() : rb;
    for (const QImage &src : overlays) {
        if (src.isNull()) continue;
        const QImage o = src.convertToFormat(QImage::Format_ARGB32);
        const double ro = double(o.width()) / o.height();
        QPainter p(&out);
        p.setOpacity(opacityPct / 100.0);
        p.setRenderHint(QPainter::SmoothPixmapTransform, false);
        const bool perFrame = anim.frames() > 1 && qAbs(ro - rf) < 0.01 && qAbs(ro - rb) >= 0.01;
        if (perFrame) {
            for (int i = 0; i < anim.frames(); ++i)
                p.drawImage(QRect((i % anim.cols) * fs.width(), (i / anim.cols) * fs.height(), fs.width(), fs.height()), o);
        } else {
            p.drawImage(QRect(0, 0, out.width(), out.height()), o);
        }
    }
    return out;
}

// ------------------------------------------------------------------ aperçu

// Mis à true quand une image animée vient d'être dessinée : le panneau d'aperçu sait alors qu'il doit se redessiner.
inline bool &animatedSeen()
{
    static bool b = false;
    return b;
}

inline QImage loadImg(const QString &path)
{
    struct E { QImage img; qint64 t; };
    static QHash<QString, E> cache;
    const QFileInfo fi(path);
    if (path.isEmpty() || !fi.exists()) return QImage();
    const qint64 t = fi.lastModified().toMSecsSinceEpoch() ^ fi.size();
    const auto it = cache.constFind(path);
    if (it != cache.constEnd() && it->t == t) return it->img;
    QImage im(path);
    if (im.isNull()) return QImage();
    im = im.convertToFormat(QImage::Format_ARGB32);
    if (cache.size() > 400) cache.clear();
    cache.insert(path, E{im, t});
    return im;
}

// Texture finale (base + textures par-dessus) de la clé `key`, image entière (bande ou planche comprise).
inline QImage composedImage(const QVariantMap &m, const QString &key, bool overlays, Anim *animOut = nullptr)
{
    const QString path = m.value(key).toString();
    const QImage base = loadImg(path);
    if (base.isNull()) {
        if (animOut) *animOut = Anim();
        return QImage();
    }
    const Anim a = animFor(m, base);
    if (animOut) *animOut = a;
    if (!overlays) return base;
    QList<QImage> ovs;
    QString sig;
    for (const char *k : {"overlay", "overlay2"}) {
        const QString op = m.value(k).toString();
        if (op.isEmpty()) continue;
        const QImage oi = loadImg(op);
        if (oi.isNull()) continue;
        ovs << oi;
        sig += op + "@" + QString::number(QFileInfo(op).lastModified().toMSecsSinceEpoch()) + "|";
    }
    if (ovs.isEmpty()) return base;
    // petit cache : la fusion est refaite seulement si une entrée change
    struct E { QImage img; };
    static QHash<QString, E> cache;
    const QString ck = path + "@" + QString::number(QFileInfo(path).lastModified().toMSecsSinceEpoch()) + "|" + sig +
                       QString::number(opacityOf(m)) + "|" + QString::number(int(a.type)) + "x" + QString::number(a.cols) + "x" +
                       QString::number(a.rows);
    const auto it = cache.constFind(ck);
    if (it != cache.constEnd()) return it->img;
    const QImage out = compose(base, ovs, opacityOf(m), a);
    if (cache.size() > 100) cache.clear();
    cache.insert(ck, E{out});
    return out;
}

// Image à dessiner dans un aperçu : textures fusionnées, et pour une animation l'image courante
// (animate = true : elle change avec le temps) ou la première (animate = false : icônes).
inline QImage previewImage(const QVariantMap &m, const QString &key, bool overlays, bool animate)
{
    Anim a;
    const QImage full = composedImage(m, key, overlays, &a);
    if (full.isNull() || a.type == Anim::None) return full;
    if (animate) animatedSeen() = true;
    return frame(full, a, animate ? currentFrame(a) : 0);
}

// Une image qui ressemble à une bande verticale (utilisée pour les textures de plugins, sans réglage) : première image.
inline QImage autoFirstFrame(const QImage &img)
{
    const int w = img.width(), h = img.height();
    if (w > 0 && h > w && h % w == 0 && h / w <= 4096) return img.copy(0, 0, w, w);
    return img;
}

} // namespace lctx

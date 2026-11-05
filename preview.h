#pragma once
// Aperçus visuels : résolution des textures, cube isométrique, coupe de biome,
// grille de craft, etc. Tout est dessiné avec QPainter (aucune dépendance OpenGL).

#include <QtWidgets>
#include "generator.h"
#include "mesh.h"
#include "texcompose.h"

// ------------------------------------------------------------ contexte projet

struct PCtx {
    Tabs *tabs = nullptr;
    QString modid;
    std::function<QString(const QString &)> pluginTexture; // chemin PNG fourni par un plugin
};
inline PCtx &pctx()
{
    static PCtx c;
    return c;
}

// ------------------------------------------------------------------ textures

inline QImage unknownTexture()
{
    static QImage img;
    if (img.isNull()) {
        img = QImage(16, 16, QImage::Format_ARGB32);
        for (int y = 0; y < 16; ++y)
            for (int x = 0; x < 16; ++x)
                img.setPixelColor(x, y, (((x / 8) + (y / 8)) % 2) ? QColor(0, 0, 0) : QColor(255, 0, 255));
    }
    return img;
}

// Texture d'un élément pour un aperçu : textures par-dessus fusionnées, image courante si animée.
// animate = false : toujours la première image (objets, icônes).
inline QImage texOf(const QVariantMap &m, const QString &key, bool overlays = true, bool animate = true)
{
    const QImage im = lctx::previewImage(m, key, overlays, animate);
    return im.isNull() ? unknownTexture() : im;
}

// Texture procédurale pour les blocs du jeu de base (default:stone, etc.)
inline QImage procTexture(const QString &name)
{
    static QHash<QString, QImage> cache;
    if (cache.contains(name)) return cache.value(name);

    QString n = name.toLower();
    if (n.contains(':')) n = n.section(':', 1);

    struct C { const char *k; int r, g, b; };
    static const C tbl[] = {
        {"dirt_with_grass", 110, 80, 50}, {"dirt", 110, 80, 50},   {"desert_sand", 205, 150, 90},
        {"sandstone", 215, 190, 140},     {"sand", 222, 205, 150}, {"snow", 240, 245, 250},
        {"ice", 170, 210, 240},           {"water", 50, 90, 200},  {"lava", 230, 100, 20},
        {"leaves", 50, 130, 40},          {"tree", 110, 80, 45},   {"wood", 170, 125, 70},
        {"gravel", 130, 125, 120},        {"cobble", 110, 110, 110}, {"stone", 125, 125, 125},
        {"glass", 200, 230, 240},         {"coal", 60, 60, 60},    {"iron", 190, 160, 140},
        {"diamond", 90, 210, 220},        {"gold", 230, 200, 60},  {"copper", 200, 120, 80},
        {"mese", 230, 220, 60},           {"torch", 250, 200, 80}, {"clay", 160, 160, 180},
        {"brick", 150, 70, 60},           {"obsidian", 30, 20, 40}, {"cactus", 40, 120, 60},
        {"grass", 90, 160, 60},           {"stick", 140, 100, 50}, {"steel", 200, 200, 210},
        {"bronze", 190, 120, 60},         {"apple", 200, 40, 40},  {"bread", 200, 160, 90},
    };
    static const C colors[] = {
        {"white", 235, 235, 235}, {"light_blue", 110, 170, 230}, {"lightblue", 110, 170, 230},
        {"orange", 230, 130, 40}, {"magenta", 200, 70, 190},    {"yellow", 235, 215, 50},
        {"lime", 120, 200, 40},   {"pink", 240, 150, 180},      {"dark_grey", 80, 80, 85},
        {"dark_gray", 80, 80, 85}, {"grey", 140, 140, 145},     {"gray", 140, 140, 145},
        {"cyan", 40, 160, 170},   {"violet", 120, 50, 190},     {"purple", 120, 50, 190},
        {"blue", 50, 70, 200},    {"brown", 110, 70, 40},       {"dark_green", 40, 110, 40},
        {"green", 60, 170, 50},   {"red", 200, 40, 40},         {"black", 25, 25, 28},
    };
    const size_t hash = qHash(name);
    QColor base = QColor::fromHsv(int(hash % 360), 110, 190);
    bool matched = false;
    for (const C &c : tbl)
        if (n.contains(QLatin1String(c.k))) {
            base = QColor(c.r, c.g, c.b);
            matched = true;
            break;
        }
    if (!matched)
        for (const C &c : colors)
            if (n.contains(QLatin1String(c.k))) {
                base = QColor(c.r, c.g, c.b);
                break;
            }
    const bool grass = n.contains("dirt_with_grass");
    const bool water = n.contains("water");

    QImage img(16, 16, QImage::Format_ARGB32);
    const quint32 seed = static_cast<quint32>(hash);
    QRandomGenerator rng{seed};
    for (int y = 0; y < 16; ++y)
        for (int x = 0; x < 16; ++x) {
            const int d = rng.bounded(29) - 14;
            QColor c(qBound(0, base.red() + d, 255), qBound(0, base.green() + d, 255),
                     qBound(0, base.blue() + d, 255));
            if (grass && y < 4)
                c = QColor(qBound(0, 70 + d, 255), qBound(0, 150 + d, 255), qBound(0, 50 + d, 255));
            if (water) c.setAlpha(170);
            img.setPixelColor(x, y, c);
        }
    cache.insert(name, img);
    return img;
}

inline QImage loadCached(const QString &path)
{
    struct E { QImage img; qint64 t; };
    static QHash<QString, E> cache;
    const qint64 t = QFileInfo(path).lastModified().toMSecsSinceEpoch();
    auto it = cache.find(path);
    if (it != cache.end() && it.value().t == t) return it.value().img;
    QImage im(path);
    if (im.isNull()) return QImage();
    im = im.convertToFormat(QImage::Format_ARGB32);
    cache.insert(path, E{im, t});
    return im;
}

inline QImage imageFromPath(const QString &path)
{
    if (path.isEmpty()) return unknownTexture();
    const QImage im = loadCached(path);
    return im.isNull() ? unknownTexture() : im;
}

// Données (et clé de la texture) d'un bloc/objet du projet ; false si la référence n'est pas un élément du projet
inline bool localTextureData(const QString &ref, QVariantMap *map, QString *key)
{
    PCtx &c = pctx();
    if (!c.tabs) return false;
    QString n = ref.trimmed();
    const int colon = n.indexOf(':');
    if (colon >= 0) {
        if (n.left(colon) != c.modid) return false;
        n = n.mid(colon + 1);
    }
    const QString id = identOf(n);
    struct S { EditorTab *t; const char *key; };
    const S sources[] = {{c.tabs->nodes, "texture"}, {c.tabs->models, "tex1"}, {c.tabs->items, "texture"},
                         {c.tabs->foods, "texture"}, {c.tabs->tools, "texture"}};
    for (const S &s : sources)
        for (const QVariantMap &m : s.t->rawItems())
            if (identOf(m["name"].toString()) == id) {
                *map = m;
                *key = s.key;
                return true;
            }
    return false;
}

// Chemin de la texture d'un bloc/objet du projet (found = l'élément existe)
inline QString localTexturePath(const QString &ref, bool *found)
{
    QVariantMap m;
    QString k;
    *found = localTextureData(ref, &m, &k);
    return *found ? m[k].toString() : QString();
}

// Image d'un bloc/objet par sa référence ("mon_mod:bloc_1", "default:stone"...)
inline QImage imageForRef(const QString &ref)
{
    const QString r = ref.trimmed();
    if (r.isEmpty() || r == "air") return QImage();
    QVariantMap lm;
    QString lk;
    if (localTextureData(r, &lm, &lk)) { // élément du projet : textures par-dessus fusionnées, 1re image si animé
        const QImage im = lctx::previewImage(lm, lk, true, false);
        return im.isNull() ? unknownTexture() : im;
    }
    if (pctx().pluginTexture) {
        const QString pp = pctx().pluginTexture(r);
        if (!pp.isEmpty()) {
            const QImage im = loadCached(pp);
            if (!im.isNull()) return lctx::autoFirstFrame(im); // texture animée d'un plugin : 1re image
        }
    }
    return procTexture(r);
}

inline QIcon iconFromImage(const QImage &img, int size = 32)
{
    if (img.isNull()) return QIcon();
    return QIcon(QPixmap::fromImage(img.scaled(size, size, Qt::KeepAspectRatio, Qt::FastTransformation)));
}

// ------------------------------------------------------------- primitives

inline void drawQuadImage(QPainter &p, const QImage &img, const QPolygonF &quad)
{
    QTransform sq;
    if (img.isNull() || !QTransform::squareToQuad(quad, sq)) return;
    p.save();
    p.setTransform(QTransform::fromScale(1.0 / img.width(), 1.0 / img.height()) * sq, true);
    p.drawImage(0, 0, img);
    p.restore();
}

// Cube isométrique texturé centré sur c (a = demi-largeur)
inline void drawIsoCube(QPainter &p, QPointF c, double a, const QImage &top, const QImage &left,
                        const QImage &right, int alpha = 255)
{
    const QPointF L(c.x() - a, c.y()), T(c.x(), c.y() - a / 2), R(c.x() + a, c.y()), B(c.x(), c.y() + a / 2);
    const QPointF dh(0, a);
    const QPolygonF leftQ = QPolygonF() << L << B << (B + dh) << (L + dh);
    const QPolygonF rightQ = QPolygonF() << B << R << (R + dh) << (B + dh);
    const QPolygonF topQ = QPolygonF() << T << R << B << L;
    p.save();
    p.setOpacity(alpha / 255.0);
    drawQuadImage(p, left, leftQ);
    drawQuadImage(p, right, rightQ);
    drawQuadImage(p, top, topQ);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(0, 0, 0, 70));
    p.drawPolygon(leftQ);
    p.setBrush(QColor(0, 0, 0, 115));
    p.drawPolygon(rightQ);
    p.restore();
}

inline void drawLabel(QPainter &p, const QRect &r, const QString &t, const QColor &col = QColor(220, 224, 230),
                      bool bold = false, Qt::Alignment al = Qt::AlignHCenter | Qt::AlignTop)
{
    p.save();
    QFont f = p.font();
    f.setBold(bold);
    p.setFont(f);
    p.setPen(col);
    p.drawText(r, int(al) | Qt::TextWordWrap, t);
    p.restore();
}

inline void drawOutlined(QPainter &p, const QRect &r, const QString &t, Qt::Alignment al)
{
    p.save();
    QFont f = p.font();
    f.setBold(true);
    p.setFont(f);
    p.setPen(Qt::black);
    p.drawText(r.translated(1, 1), int(al), t);
    p.setPen(Qt::white);
    p.drawText(r, int(al), t);
    p.restore();
}

inline void drawSlot(QPainter &p, const QRect &q, const QString &ref, int count = 1)
{
    p.save();
    p.fillRect(q, QColor(70, 70, 78));
    p.setPen(QPen(QColor(24, 24, 28), 2));
    p.setBrush(Qt::NoBrush);
    p.drawRect(q);
    const QImage img = imageForRef(ref);
    if (!img.isNull()) p.drawImage(q.adjusted(5, 5, -5, -5), img);
    if (count > 1) drawOutlined(p, q.adjusted(0, 0, -3, -1), QString::number(count), Qt::AlignRight | Qt::AlignBottom);
    p.restore();
}

// ------------------------------------------------------------ zone d'aperçu

class PreviewPane : public QWidget {
public:
    using Fn = std::function<void(QPainter &, const QRect &, const QVariantMap &)>;

    PreviewPane(Fn fn, int minH = 220) : fn_(std::move(fn))
    {
        setMinimumSize(250, minH);
        // textures animées : on se redessine ~10 fois par seconde, seulement si la dernière image dessinée était animée
        auto *tm = new QTimer(this);
        tm->setInterval(100);
        connect(tm, &QTimer::timeout, this, [this] {
            if (animated_ && isVisible()) update();
        });
        tm->start();
    }

    void setData(const QVariantMap &m)
    {
        data_ = m;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.fillRect(rect(), QColor(36, 40, 48));
        p.setPen(QColor(70, 75, 85));
        p.drawRect(rect().adjusted(0, 0, -1, -1));
        if (data_.isEmpty()) {
            animated_ = false;
            p.setPen(QColor(150, 155, 165));
            p.drawText(rect(), Qt::AlignCenter, QStringLiteral("Ajoutez ou sélectionnez un élément\npour voir l'aperçu"));
            return;
        }
        lctx::animatedSeen() = false;
        fn_(p, rect(), data_);
        animated_ = lctx::animatedSeen();
    }

private:
    Fn fn_;
    QVariantMap data_;
    bool animated_ = false;
};

// ------------------------------------------------------------------ peintres

inline void paintNode(QPainter &p, const QRect &r, const QVariantMap &m)
{
    const QImage tex = texOf(m, "texture", true, true);
    const QString dt = m["drawtype"].toString();
    const int light = m["light"].toInt();
    const double a = qMin(r.width() * 0.34, r.height() * 0.30);
    const QPointF c(r.center().x(), r.top() + r.height() * 0.40);

    if (light > 0) {
        QRadialGradient g(c, a * (1.2 + light / 6.0));
        g.setColorAt(0, QColor(255, 240, 160, 60 + light * 9));
        g.setColorAt(1, QColor(255, 240, 160, 0));
        p.fillRect(r, g);
    }
    if (dt == "normal" || dt == "glasslike" || dt == "allfaces") {
        drawIsoCube(p, c, a, tex, tex, tex, dt == "normal" ? 255 : 150);
    } else if (dt == "airlike") {
        p.save();
        p.setPen(QPen(QColor(160, 165, 175), 1, Qt::DashLine));
        p.drawRect(QRectF(c.x() - a, c.y() - a / 2, a * 2, a * 1.5));
        p.restore();
    } else {
        const double s = a * 1.7;
        p.drawImage(QRectF(c.x() - s / 2, c.y() - s / 2, s, s), tex);
    }

    QStringList info;
    info << QString("Groupe : %1 (dureté %2)").arg(m["group"].toString()).arg(m["level"].toInt());
    if (light > 0) info << QString("Lumière : %1").arg(light);
    if (!m["walkable"].toBool()) info << QStringLiteral("Traversable");
    {
        lctx::Anim an;
        lctx::composedImage(m, "texture", false, &an);
        if (an.type != lctx::Anim::None)
            info << QString("Animation : %1 images, cycle de %2 s").arg(an.frames()).arg(an.time);
        if (!m["overlay"].toString().isEmpty() || !m["overlay2"].toString().isEmpty())
            info << QStringLiteral("Texture(s) par-dessus fusionnée(s)");
    }
    const int fy = int(c.y() + a * 1.5) + 12;
    drawLabel(p, QRect(r.left() + 10, fy, r.width() - 20, 24), m["description"].toString(), QColor(240, 240, 245), true);
    drawLabel(p, QRect(r.left() + 10, fy + 24, r.width() - 20, 60), info.join("\n"), QColor(170, 176, 188));
}

// kind : 0 objet, 1 nourriture, 2 outil
inline PreviewPane::Fn itemPainter(int kind)
{
    return [kind](QPainter &p, const QRect &r, const QVariantMap &m) {
        const QImage tex = texOf(m, "texture", true, false);
        const int s = qMax(48, qMin(150, qMin(r.width() - 40, r.height() / 2)));
        const QRect slot(r.center().x() - s / 2, r.top() + 18, s, s);
        p.fillRect(slot, QColor(62, 62, 68));
        p.setPen(QPen(QColor(20, 20, 24), 2));
        p.drawRect(slot);
        p.drawImage(slot.adjusted(s / 8, s / 8, -s / 8, -s / 8), tex);

        const int y = slot.bottom() + 14;
        drawLabel(p, QRect(r.left() + 10, y, r.width() - 20, 24), m["description"].toString(), QColor(240, 240, 245), true);
        QString info;
        if (kind == 0) {
            info = QString("Pile max : %1").arg(m["stack"].toInt());
        } else if (kind == 1) {
            const int hp = m["hp"].toInt();
            QString hearts = QString("♥").repeated(qMin(10, hp / 2));
            if (hp % 2) hearts += " ½";
            drawLabel(p, QRect(r.left() + 10, y + 24, r.width() - 20, 24), hearts, QColor(235, 80, 90), true);
            info = QString("Rend %1 demi-cœur(s) · pile max %2").arg(hp).arg(m["stack"].toInt());
        } else {
            info = QString("Mine : %1 (niveau max %2)\nDurabilité : %3 utilisations\nTemps (s) : %4 / %5 / %6")
                       .arg(m["group"].toString()).arg(m["level"].toInt()).arg(m["uses"].toInt())
                       .arg(m["t1"].toInt() / 10.0).arg(m["t2"].toInt() / 10.0).arg(m["t3"].toInt() / 10.0);
        }
        drawLabel(p, QRect(r.left() + 10, y + (kind == 1 ? 48 : 26), r.width() - 20, 90), info, QColor(170, 176, 188));
    };
}

inline void paintTexStrip(QPainter &p, const QRect &r, const QVariantMap &m)
{
    const int s = qMin(64, (r.width() - 50) / 4);
    int x = r.left() + 10;
    for (int i = 1; i <= 4; ++i) {
        const QString path = m[QString("tex%1").arg(i)].toString();
        const QRect q(x, r.top() + 8, s, s);
        p.fillRect(q, QColor(62, 62, 68));
        if (!path.isEmpty()) p.drawImage(q, texOf(m, QString("tex%1").arg(i), false, true));
        else {
            p.setPen(QColor(120, 125, 135));
            p.drawText(q, Qt::AlignCenter, QString::number(i));
        }
        p.setPen(QColor(20, 20, 24));
        p.drawRect(q);
        x += s + 10;
    }
}

inline void paintBiome(QPainter &p, const QRect &r, const QVariantMap &m)
{
    const int topD = qBound(0, m["depth_top"].toInt(), 5);
    const int fillD = qBound(0, m["depth_filler"].toInt(), 6);
    const int airRows = 2, stoneRows = 3, cols = 4;
    const int rows = airRows + topD + fillD + stoneRows;
    const int secH = int(r.height() * 0.50);
    int t = qMin(int((r.width() - 24) * 0.55 / cols), (secH - 10) / rows);
    t = qMax(t, 6);
    const int x0 = r.left() + 12, y0 = r.top() + 10;

    QLinearGradient sky(0, y0, 0, y0 + airRows * t);
    sky.setColorAt(0, QColor(120, 175, 235));
    sky.setColorAt(1, QColor(190, 220, 245));
    p.fillRect(QRect(x0, y0, cols * t, airRows * t), sky);

    int row = airRows;
    QFont small = p.font();
    if (small.pointSizeF() > 8) small.setPointSizeF(small.pointSizeF() - 1);
    auto layer = [&](const QString &refIn, int n, const QString &what) {
        if (n <= 0) return;
        const QString ref = refIn.trimmed();
        const QImage img = imageForRef(ref);
        for (int i = 0; i < n; ++i)
            for (int c = 0; c < cols; ++c) {
                const QRect q(x0 + c * t, y0 + (row + i) * t, t, t);
                if (img.isNull()) p.fillRect(q, QColor(90, 90, 95));
                else p.drawImage(q, img);
            }
        p.save();
        p.setFont(small);
        p.setPen(QColor(220, 224, 230));
        const QString nm = ref.isEmpty() ? QStringLiteral("(par défaut)") : ref.section(':', -1);
        const int lx = x0 + cols * t + 8;
        p.drawText(QRect(lx, y0 + row * t, r.right() - lx - 4, n * t), Qt::AlignVCenter | Qt::TextWordWrap,
                   what + "\n" + nm);
        p.restore();
        row += n;
    };
    layer(m["node_top"].toString(), topD, QString("surface ×%1").arg(topD));
    layer(m["node_filler"].toString(), fillD, QString("remplissage ×%1").arg(fillD));
    const QString stone = m["node_stone"].toString().trimmed();
    layer(stone.isEmpty() ? QStringLiteral("default:stone") : stone, stoneRows, QStringLiteral("roche"));
    p.setPen(QColor(0, 0, 0, 140));
    p.drawRect(QRect(x0, y0, cols * t, rows * t));

    const int infoY = y0 + rows * t + 6;
    drawLabel(p, QRect(r.left() + 8, infoY, r.width() - 16, 34),
              QString("Altitude %1 → %2  ·  chaleur %3  ·  humidité %4")
                  .arg(m["ymin"].toInt()).arg(m["ymax"].toInt()).arg(m["heat"].toInt()).arg(m["humid"].toInt()),
              QColor(170, 176, 188));

    // carte climat (température / humidité) avec tous les biomes du projet
    const int my = infoY + 52;
    const int s = qMin(r.width() - 40, r.bottom() - my - 14);
    if (s < 50) return;
    const int mx = r.center().x() - s / 2;
    auto mix = [](const QColor &a, const QColor &b, double k) {
        return QColor(int(a.red() + (b.red() - a.red()) * k), int(a.green() + (b.green() - a.green()) * k),
                      int(a.blue() + (b.blue() - a.blue()) * k));
    };
    const QColor dryHot(232, 200, 120), wetHot(50, 150, 60), dryCold(190, 205, 215), wetCold(70, 120, 130);
    QImage clim(32, 32, QImage::Format_ARGB32);
    for (int gy = 0; gy < 32; ++gy)
        for (int gx = 0; gx < 32; ++gx) {
            const double heat = 1.0 - gy / 31.0, hum = gx / 31.0;
            clim.setPixelColor(gx, gy, mix(mix(dryCold, wetCold, hum), mix(dryHot, wetHot, hum), heat));
        }
    p.save();
    p.setRenderHint(QPainter::SmoothPixmapTransform, true);
    p.drawImage(QRect(mx, my, s, s), clim);
    p.restore();
    p.setPen(QColor(0, 0, 0, 140));
    p.drawRect(QRect(mx, my, s, s));
    drawLabel(p, QRect(mx, my - 18, s, 16), QStringLiteral("↑ température        humidité →"), QColor(170, 176, 188));

    auto dot = [&](const QVariantMap &b, bool sel) {
        const QPointF c(mx + b["humid"].toInt() / 100.0 * s, my + (1.0 - b["heat"].toInt() / 100.0) * s);
        p.save();
        p.setRenderHint(QPainter::Antialiasing, true);
        p.setPen(QPen(sel ? QColor(255, 255, 255) : QColor(30, 30, 30), sel ? 2 : 1));
        p.setBrush(sel ? QColor(235, 70, 70) : QColor(250, 250, 250));
        p.drawEllipse(c, sel ? 7.0 : 4.0, sel ? 7.0 : 4.0);
        p.setPen(Qt::black);
        p.drawText(c + QPointF(9, 4), b["name"].toString());
        p.setPen(Qt::white);
        p.drawText(c + QPointF(8, 3), b["name"].toString());
        p.restore();
    };
    if (pctx().tabs)
        for (const QVariantMap &b : pctx().tabs->biomes->rawItems())
            if (b["name"].toString() != m["name"].toString()) dot(b, false);
    dot(m, true);
}

inline void paintCraft(QPainter &p, const QRect &r, const QVariantMap &m)
{
    const QString kind = m["kind"].toString();
    const int gap = 4;
    const int s = qBound(18, (r.width() - 70) / 4, 56);
    const int gridW = 3 * s + 2 * gap;
    const int total = gridW + 44 + s;
    const int x0 = r.center().x() - total / 2;
    const int y0 = r.top() + 28;
    const int midY = y0 + gridW / 2;

    const QRect arrow(x0 + gridW, midY - s / 2, 44, s);
    const QRect outSlot(x0 + gridW + 44, midY - s / 2, s, s);

    if (kind == "cooking" || kind == "fuel") {
        drawSlot(p, QRect(x0 + gridW / 2 - s / 2, midY - s / 2, s, s), m["r00"].toString());
    } else {
        QStringList packed;
        if (kind == "shapeless")
            for (int i = 0; i < 9; ++i) {
                const QString v = m[QString("r%1%2").arg(i / 3).arg(i % 3)].toString();
                if (!v.trimmed().isEmpty()) packed << v;
            }
        for (int i = 0; i < 9; ++i) {
            const QString ref = (kind == "shapeless") ? (i < packed.size() ? packed[i] : QString())
                                                      : m[QString("r%1%2").arg(i / 3).arg(i % 3)].toString();
            drawSlot(p, QRect(x0 + (i % 3) * (s + gap), y0 + (i / 3) * (s + gap), s, s), ref);
        }
    }

    if (kind != "fuel") {
        p.save();
        QFont f = p.font();
        f.setPointSize(20);
        p.setFont(f);
        p.setPen(QColor(200, 204, 212));
        p.drawText(arrow, Qt::AlignCenter, kind == "cooking" ? QStringLiteral("→") : QStringLiteral("→"));
        p.restore();
        if (m["output"].toString().trimmed().isEmpty()) {
            p.save();
            p.setPen(QPen(QColor(200, 100, 100), 1, Qt::DashLine));
            p.drawRect(outSlot);
            p.restore();
        } else {
            drawSlot(p, outSlot, m["output"].toString(), m["count"].toInt());
        }
    }

    QString info;
    if (kind == "shaped") info = QStringLiteral("Craft avec forme (3×3)");
    else if (kind == "shapeless") info = QStringLiteral("Craft sans forme (ordre libre)");
    else if (kind == "cooking") info = QString("Cuisson au four : %1 s").arg(m["time"].toInt());
    else info = QString("Combustible : brûle %1 s").arg(m["time"].toInt());
    if (kind != "fuel" && m["output"].toString().trimmed().isEmpty()) info += QStringLiteral("\nRésultat manquant");
    drawLabel(p, QRect(r.left() + 10, y0 + gridW + 20, r.width() - 20, 60), info, QColor(170, 176, 188));
}

inline void paintOre(QPainter &p, const QRect &r, const QVariantMap &m)
{
    const int cols = 12, rows = 8;
    const int t = qMax(8, qMin((r.width() - 24) / cols, (r.height() - 90) / rows));
    const int x0 = r.center().x() - cols * t / 2, y0 = r.top() + 12;
    const QImage host = imageForRef(m["wherein"].toString());
    const QImage ore = imageForRef(m["ore"].toString());
    for (int y = 0; y < rows; ++y)
        for (int x = 0; x < cols; ++x) {
            const QRect q(x0 + x * t, y0 + y * t, t, t);
            if (host.isNull()) p.fillRect(q, QColor(100, 100, 105));
            else p.drawImage(q, host);
        }
    const quint32 seed = static_cast<quint32>(qHash(m["name"].toString()));
    QRandomGenerator rng{seed};
    const int num = qBound(1, m["num"].toInt(), 14);
    const int rad = qBound(1, m["size"].toInt() / 2 + 1, 4);
    const int cx = 3 + rng.bounded(cols - 6), cy = 2 + rng.bounded(rows - 4);
    QSet<int> cells;
    for (int guard = 0; cells.size() < num && guard < 300; ++guard) {
        const int x = cx + rng.bounded(2 * rad + 1) - rad, y = cy + rng.bounded(2 * rad + 1) - rad;
        if (x >= 0 && y >= 0 && x < cols && y < rows) cells.insert(y * cols + x);
    }
    for (int c : cells) {
        const QRect q(x0 + (c % cols) * t, y0 + (c / cols) * t, t, t);
        if (!ore.isNull()) p.drawImage(q, ore);
    }
    p.setPen(QColor(0, 0, 0, 140));
    p.drawRect(QRect(x0, y0, cols * t, rows * t));
    drawLabel(p, QRect(r.left() + 10, y0 + rows * t + 10, r.width() - 20, 70),
              QString("1 minerai tous les %1 nodes³\nVeine de %2 minerais (taille %3)\nAltitude %4 → %5")
                  .arg(m["scarcity"].toInt()).arg(m["num"].toInt()).arg(m["size"].toInt())
                  .arg(m["ymin"].toInt()).arg(m["ymax"].toInt()),
              QColor(170, 176, 188));
}

inline void paintDeco(QPainter &p, const QRect &r, const QVariantMap &m)
{
    const int cols = 8;
    const int t = qMax(10, qMin(44, (r.width() - 24) / cols));
    const int x0 = r.center().x() - cols * t / 2;
    const int groundY = r.top() + 20 + 3 * t;
    QLinearGradient sky(0, r.top() + 20, 0, groundY);
    sky.setColorAt(0, QColor(120, 175, 235));
    sky.setColorAt(1, QColor(190, 220, 245));
    p.fillRect(QRect(x0, r.top() + 20, cols * t, 3 * t), sky);

    const QString first = m["place_on"].toString().split(',', Qt::SkipEmptyParts).value(0).trimmed();
    const QImage ground = imageForRef(first);
    for (int row = 0; row < 2; ++row)
        for (int c = 0; c < cols; ++c) {
            const QRect q(x0 + c * t, groundY + row * t, t, t);
            if (ground.isNull()) p.fillRect(q, QColor(110, 80, 50));
            else p.drawImage(q, ground);
        }
    const QImage deco = imageForRef(m["deco"].toString());
    const int n = qBound(1, m["fill"].toInt() / 8 + 1, 7);
    const quint32 seed = static_cast<quint32>(qHash(m["name"].toString()));
    QRandomGenerator rng{seed};
    QSet<int> used;
    for (int i = 0; i < n * 3 && used.size() < n; ++i) used.insert(rng.bounded(cols));
    for (int c : used)
        if (!deco.isNull()) p.drawImage(QRect(x0 + c * t, groundY - t, t, t), deco);
    p.setPen(QColor(0, 0, 0, 140));
    p.drawRect(QRect(x0, r.top() + 20, cols * t, 5 * t));
    drawLabel(p, QRect(r.left() + 10, groundY + 2 * t + 12, r.width() - 20, 60),
              QString("Densité : %1 ‰\nAltitude %2 → %3").arg(m["fill"].toInt()).arg(m["ymin"].toInt()).arg(m["ymax"].toInt()),
              QColor(170, 176, 188));
}

inline void paintEntity(QPainter &p, const QRect &r, const QVariantMap &m)
{
    const QImage tex = imageFromPath(m["texture"].toString());
    const int s = qBound(40, qMin(r.width() - 40, r.height() - 90), 110);
    const QRect q(r.center().x() - s / 2, r.top() + 10, s, s);
    p.fillRect(q, QColor(62, 62, 68));
    p.drawImage(q.adjusted(4, 4, -4, -4), tex);
    const QString beh = m["behaviour"].toString();
    const QString bname = beh == "wander" ? QStringLiteral("erre au hasard")
                          : beh == "follow" ? QStringLiteral("suit le joueur") : QStringLiteral("immobile");
    drawLabel(p, QRect(r.left() + 10, q.bottom() + 8, r.width() - 20, 60),
              QString("%1 PV · %2 · vitesse %3\napparence : %4").arg(m["hp"].toInt()).arg(bname)
                  .arg(m["speed"].toDouble()).arg(m["visual"].toString()),
              QColor(170, 176, 188));
}

inline void paintCommand(QPainter &p, const QRect &r, const QVariantMap &m)
{
    const QRect box(r.left() + 10, r.top() + 20, r.width() - 20, 130);
    p.fillRect(box, QColor(15, 17, 22));
    p.setPen(QColor(70, 75, 85));
    p.drawRect(box);
    const QString line1 = QString("<Joueur> /%1 %2").arg(m["name"].toString(), m["params"].toString());
    QString msg = m["message"].toString();
    msg.replace("{player}", "Joueur");
    drawLabel(p, box.adjusted(8, 8, -8, 0), line1, QColor(235, 235, 240), false, Qt::AlignLeft | Qt::AlignTop);
    drawLabel(p, box.adjusted(8, 38, -8, 0), msg, QColor(150, 220, 150), false, Qt::AlignLeft | Qt::AlignTop);
    const QString priv = m["priv"].toString().trimmed();
    drawLabel(p, QRect(r.left() + 10, box.bottom() + 12, r.width() - 20, 40),
              priv.isEmpty() ? QStringLiteral("Accessible à tous les joueurs")
                             : QString("Privilège requis : %1").arg(priv),
              QColor(170, 176, 188));
}

// Crée un panneau d'aperçu relié à un éditeur (mise à jour en direct)
inline PreviewPane *attachPreview(EditorTab *t, PreviewPane::Fn fn, int minH = 220)
{
    auto *pane = new PreviewPane(std::move(fn), minH);
    t->setSide(pane);
    t->onChanged = [pane](const QVariantMap &m) { pane->setData(m); };
    return pane;
}

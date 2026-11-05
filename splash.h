#pragma once
// Cadre de chargement affiché au lancement de LuantiCreator pendant que l'application se prépare
// (thème, plugins, plugins intégrés, scripts de démarrage, icônes, textures, lanceur), puis refermé en fondu
// pour laisser place au lanceur. Au démarrage de l'application il reste affiché au moins 3 s (voir main()) ;
// le même cadre sert aussi, en version courte, à l'ouverture d'une fenêtre ou au chargement d'un mod (LcSplash::busy).
// Le cadre reste affiché au moins `minMs` millisecondes :
// la barre progresse en fonction du travail réellement terminé, sans jamais dépasser le temps écoulé.
//
// Utilisation :
//   LcSplash splash;
//   splash.run({{"Chargement des plugins", [](LcSplash &s) { ...; s.setDetail("..."); s.setStepFraction(0.5); }, 3}, ...});
//   (le cadre est fermé à la fin de run())
//
//   LcSplash::busy("Ouverture du mod", [&] { w = new MaFenetre; });   // cadre le temps de construire une fenêtre
//   w->show();                                                          // (à afficher APRÈS : le cadre est au premier plan)

#include <QtWidgets>
#include <functional>

#include "appcore.h"
#include "uihooks.h"

class LcSplash : public QWidget {
public:
    struct Step {
        QString label;                         // « Chargement des plugins »
        std::function<void(LcSplash &)> run;   // travail de l'étape (synchrone)
        double weight = 1;                     // poids dans la barre de progression
    };

    LcSplash()
        : QWidget(nullptr, Qt::SplashScreen | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint)
    {
        setAttribute(Qt::WA_TranslucentBackground);
        setAttribute(Qt::WA_ShowWithoutActivating);
        setFixedSize(580, 276);
        const bool dark = QSettings("LuantiCreator", "LuantiCreator").value("dark", true).toBool();
        colors_ = lcBaseColors(dark);
        QScreen *sc = QGuiApplication::primaryScreen();
        if (QWidget *aw = QApplication::activeWindow()) // sur l'écran de la fenêtre en cours, pas forcément l'écran principal
            if (QScreen *as = aw->screen()) sc = as;
        if (sc) move(sc->availableGeometry().center() - rect().center());
        tick_.setInterval(16);
        QObject::connect(&tick_, &QTimer::timeout, this, [this] { tick(); });
    }

    // texte secondaire sous la barre (nom du plugin en cours, etc.)
    void setDetail(const QString &d)
    {
        if (cur_ >= 0 && cur_ < recs_.size()) recs_[cur_].detail = d;
        pump();
    }

    // avancement de l'étape en cours, de 0 à 1 (la barre avance en proportion du poids de l'étape)
    void setStepFraction(double f)
    {
        stepFrac_ = qBound(0.0, f, 1.0);
        target_ = qBound(0.0, (doneWeight_ + stepWeight_ * stepFrac_) / qMax(1e-9, totalWeight_), 1.0);
        pump();
    }

    // Un cadre est-il déjà affiché ? (un cadre ponctuel demandé pendant un autre s'exécute simplement sans s'afficher)
    static bool &active()
    {
        static bool a = false;
        return a;
    }

    // Cadre de chargement ponctuel (ouverture d'une fenêtre, chargement d'un mod) : exécute `work` pendant que le cadre
    // est affiché, au moins `minMs` ms, puis le ferme en fondu. À appeler AVANT de montrer la fenêtre construite.
    static void busy(const QString &label, const std::function<void()> &work, const QString &detail = QString(), int minMs = 600)
    {
        if (active() || !qApp) { // déjà dans un cadre (démarrage, rechargement) : pas de second cadre par-dessus
            work();
            return;
        }
        LcSplash sp;
        sp.compact_ = true;
        QList<Step> steps;
        steps << Step{label, [&work, &detail](LcSplash &s) {
            if (!detail.isEmpty()) s.setDetail(detail);
            work();
        }, 1};
        sp.run(steps, minMs);
    }

    // Exécute les étapes dans l'ordre, attend la durée minimale puis ferme le cadre en fondu.
    void run(const QList<Step> &steps, int minMs = 3000)
    {
        struct Guard {
            Guard() { active() = true; }
            ~Guard() { active() = false; }
        } guard;
        minMs_ = qMax(1, minMs);
        clock_.start();
        totalWeight_ = 0;
        for (const Step &s : steps) totalWeight_ += qMax(0.01, s.weight);
        recs_.clear();
        double acc = 0;
        for (const Step &s : steps) { // bornes de chaque étape dans la barre : l'affichage suit la barre, pas le travail réel
            const double w = qMax(0.01, s.weight) / totalWeight_;
            recs_.append({s.label, QString(), acc, acc + w});
            acc += w;
        }
        show();
        tick_.start();
        pump();
        for (int i = 0; i < steps.size(); ++i) {
            const Step &s = steps[i];
            cur_ = i;
            stepWeight_ = qMax(0.01, s.weight);
            stepFrac_ = 0;
            target_ = doneWeight_ / totalWeight_;
            pump();
            if (s.run) s.run(*this);
            doneWeight_ += stepWeight_;
            stepFrac_ = 0;
            target_ = qBound(0.0, doneWeight_ / totalWeight_, 1.0);
            pump();
        }
        cur_ = steps.size() - 1;
        target_ = 1.0;
        // attend que la barre soit pleine ET que la durée minimale soit écoulée
        QEventLoop loop;
        QTimer wait;
        wait.setInterval(16);
        QObject::connect(&wait, &QTimer::timeout, &loop, [&] {
            tick();
            if (shown_ >= 0.999 && clock_.elapsed() >= minMs_) loop.quit();
        });
        wait.start();
        loop.exec();
        wait.stop();
        shown_ = 1.0;
        update();
        fadeOutAndClose();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        const QColor surface(colors_["surface"]), border(colors_["border"]), text(colors_["text"]), muted(colors_["muted"]);
        const QColor accent(colors_["accent"]), accent2(colors_["accentHover"]), track(colors_["surface2"]);

        // ombre douce
        const QRectF card = QRectF(rect()).adjusted(14, 12, -14, -16);
        for (int i = 9; i >= 1; --i) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(0, 0, 0, 7));
            p.drawRoundedRect(card.adjusted(-i, -i + 4, i, i + 4), 16 + i, 16 + i);
        }
        p.setBrush(surface);
        p.setPen(QPen(border, 1));
        p.drawRoundedRect(card, 16, 16);

        // logo : icône « app » (celle d'un plugin, sinon celle de l'application) ; cube isométrique tant qu'aucune n'est chargée
        const QPointF c(card.left() + 74, card.top() + 74);
        const QIcon appIcon = UiHooks::get("app");
        if (!appIcon.isNull()) {
            const QPixmap px = appIcon.pixmap(QSize(72, 72));
            if (!px.isNull()) {
                const QSizeF sz = QSizeF(px.size()) / px.devicePixelRatio();
                p.setRenderHint(QPainter::SmoothPixmapTransform);
                p.drawPixmap(QRectF(c.x() - sz.width() / 2, c.y() - sz.height() / 2, sz.width(), sz.height()), px, QRectF(px.rect()));
            }
        } else {
        const double s = 30, w = s * 0.866;
        auto poly = [&](QList<QPointF> pts, const QColor &col) {
            p.setPen(Qt::NoPen);
            p.setBrush(col);
            p.drawPolygon(QPolygonF(pts.toVector()));
        };
        poly({{c.x(), c.y() - s}, {c.x() + w, c.y() - s / 2}, {c.x(), c.y()}, {c.x() - w, c.y() - s / 2}}, accent.lighter(130));
        poly({{c.x() - w, c.y() - s / 2}, {c.x(), c.y()}, {c.x(), c.y() + s}, {c.x() - w, c.y() + s / 2}}, accent);
        poly({{c.x(), c.y()}, {c.x() + w, c.y() - s / 2}, {c.x() + w, c.y() + s / 2}, {c.x(), c.y() + s}}, accent.darker(135));
        p.setPen(QPen(QColor(255, 255, 255, 70), 1));
        p.drawLine(QPointF(c.x() - w / 2, c.y() - s * 0.75), QPointF(c.x() + w / 2, c.y() - s * 0.25));
        p.drawLine(QPointF(c.x() + w / 2, c.y() - s * 0.75), QPointF(c.x() - w / 2, c.y() - s * 0.25));
        }

        // titre
        const double tx = card.left() + 128;
        QFont f = font();
        f.setPixelSize(30);
        f.setBold(true);
        p.setFont(f);
        p.setPen(text);
        p.drawText(QPointF(tx, card.top() + 70), "LuantiCreator");
        f.setBold(false);
        f.setPixelSize(13);
        p.setFont(f);
        p.setPen(muted);
#ifdef LC_VERSION
        const QString ver = tr("Créateur visuel de mods pour Luanti — version %1").arg(LC_VERSION);
#else
        const QString ver = tr("Créateur visuel de mods pour Luanti");
#endif
        p.drawText(QPointF(tx, card.top() + 94), ver);

        // étape affichée = celle où en est la barre (le travail réel peut être en avance sur l'animation)
        int di = 0;
        while (di < recs_.size() - 1 && shown_ >= recs_[di].b - 1e-9) ++di;
        const bool ready = shown_ >= 0.999;
        const QString label_ = ready ? tr("Prêt") : (di < recs_.size() ? recs_[di].label : tr("Démarrage")), detail_ = ready || di >= recs_.size() ? QString() : recs_[di].detail;
        const int index_ = di + 1, count_ = recs_.size();
        // étape en cours + compteur
        const double bx = card.left() + 40, bw = card.width() - 80, by = card.top() + 160;
        f.setPixelSize(15);
        f.setBold(true);
        p.setFont(f);
        p.setPen(text);
        const QString lab = label_ + (ready ? "" : "…");
        p.drawText(QRectF(bx, by - 34, bw - 70, 22), Qt::AlignLeft | Qt::AlignVCenter, QFontMetrics(f).elidedText(lab, Qt::ElideRight, int(bw - 70)));
        f.setBold(false);
        f.setPixelSize(12);
        p.setFont(f);
        p.setPen(muted);
        if (!recs_.isEmpty() && !compact_) p.drawText(QRectF(bx + bw - 70, by - 34, 70, 22), Qt::AlignRight | Qt::AlignVCenter, QString("%1 / %2").arg(index_).arg(count_));

        // barre de progression
        p.setPen(Qt::NoPen);
        p.setBrush(track);
        p.drawRoundedRect(QRectF(bx, by, bw, 10), 5, 5);
        const double fw = bw * qBound(0.0, shown_, 1.0);
        if (fw > 1) {
            QLinearGradient g(bx, 0, bx + bw, 0);
            g.setColorAt(0, accent);
            g.setColorAt(1, accent2);
            p.setBrush(g);
            p.drawRoundedRect(QRectF(bx, by, qMax(10.0, fw), 10), 5, 5);
            // reflet qui glisse sur la partie remplie
            if (shown_ < 0.999) {
                const double ph = std::fmod(clock_.elapsed() / 900.0, 1.0);
                const double hx = bx + (qMax(10.0, fw) + 60) * ph - 60;
                p.save();
                p.setClipRect(QRectF(bx, by, qMax(10.0, fw), 10));
                QLinearGradient hg(hx, 0, hx + 60, 0);
                hg.setColorAt(0, QColor(255, 255, 255, 0));
                hg.setColorAt(0.5, QColor(255, 255, 255, 70));
                hg.setColorAt(1, QColor(255, 255, 255, 0));
                p.setBrush(hg);
                p.drawRect(QRectF(hx, by, 60, 10));
                p.restore();
            }
        }

        // détail
        p.setPen(muted);
        p.drawText(QRectF(bx, by + 18, bw - 50, 20), Qt::AlignLeft | Qt::AlignVCenter,
                   QFontMetrics(f).elidedText(detail_, Qt::ElideMiddle, int(bw - 50)));
        p.drawText(QRectF(bx + bw - 50, by + 18, 50, 20), Qt::AlignRight | Qt::AlignVCenter, QString("%1 %").arg(int(qBound(0.0, shown_, 1.0) * 100 + 0.5)));
    }

private:
    // Fait avancer la barre affichée : vers l'avancement réel, sans dépasser le temps écoulé / durée minimale.
    void tick()
    {
        const double cap = qMin(1.0, double(clock_.elapsed()) / minMs_);
        const double goal = qMin(target_, cap);
        if (goal > shown_) shown_ = qMin(goal, shown_ + 0.035); // montée fluide
        update();
    }

    void pump()
    {
        tick();
        repaint();
        QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    }

    void fadeOutAndClose()
    {
        QEventLoop loop;
        auto *a = new QVariantAnimation(this);
        a->setDuration(180);
        a->setStartValue(1.0);
        a->setEndValue(0.0);
        QObject::connect(a, &QVariantAnimation::valueChanged, this, [this](const QVariant &v) { setWindowOpacity(v.toDouble()); });
        QObject::connect(a, &QVariantAnimation::finished, &loop, &QEventLoop::quit);
        a->start();
        loop.exec();
        tick_.stop();
        close();
    }

    struct Rec {
        QString label, detail;
        double a, b; // début et fin de l'étape dans la barre (0..1)
    };
    LcColors colors_;
    QVector<Rec> recs_;
    QTimer tick_;
    QElapsedTimer clock_;
    bool compact_ = false; // cadre ponctuel : pas de compteur « n / N »
    int minMs_ = 3000, cur_ = -1;
    double totalWeight_ = 1, doneWeight_ = 0, stepWeight_ = 1, stepFrac_ = 0, target_ = 0, shown_ = 0;
};

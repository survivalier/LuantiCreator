#pragma once
// Cadre de fenêtre personnalisé, dans le style du cadre de chargement (splash.h) : fenêtre sans bordure système,
// fond opaque, coins arrondis et fine bordure, barre de titre maison (icône / favicon, titre, boutons
// réduire, agrandir / restaurer, fermer ; glisser = déplacer, double-clic = agrandir, bords = redimensionner).
//
// Utilisation (avant d'afficher la fenêtre) :
//     LcChrome::install(this);                    // QMainWindow ou QDialog : à appeler dans le constructeur
//     LcDialog dlg(parent);                       // boîte de dialogue : même chose que QDialog + le cadre
// Le titre et l'icône de la fenêtre (setWindowTitle / setWindowIcon, y compris les icônes de plugin) sont suivis automatiquement.
// Les couleurs viennent de la palette de l'application, donc du thème sombre / clair et des plugins.
//
// Pour retrouver les cadres natifs du système : variable d'environnement LC_NATIVE_FRAMES=1, ou « customFrames » = false
// dans les réglages (QSettings "LuantiCreator"). Coins carrés : kRadius = 0.

#include <QtWidgets>
#include <functional>

// Aide : fonction appelée par le bouton « ? » de la barre de titre et par F1 (branchée dans main() sur la fenêtre de documentation).
// Argument : titre de la section à afficher (propriété « lcHelpTopic » de la fenêtre, vide = début de la documentation).
// Une fenêtre qui est elle-même l'aide met la propriété « lcNoHelp » à true.
inline std::function<void(const QString &)> &lcHelpHook()
{
    static std::function<void(const QString &)> h;
    return h;
}
inline void lcOpenHelp(QWidget *win)
{
    if (lcHelpHook() && win) lcHelpHook()(win->property("lcHelpTopic").toString());
}

inline bool lcCustomFrames()
{
    static const bool on = qEnvironmentVariableIsEmpty("LC_NATIVE_FRAMES") &&
                           QSettings("LuantiCreator", "LuantiCreator").value("customFrames", true).toBool();
    return on;
}

// ------------------------------------------------------------------ boutons réduire / agrandir / fermer
class LcCaptionButton : public QAbstractButton {
public:
    enum Kind { Help, Minimize, Maximize, Restore, Close };
    explicit LcCaptionButton(Kind k, QWidget *parent = nullptr) : QAbstractButton(parent), kind_(k)
    {
        setAttribute(Qt::WA_Hover);
        setFocusPolicy(Qt::NoFocus);
        setCursor(Qt::ArrowCursor);
        setFixedSize(40, 30);
    }
    void setKind(Kind k)
    {
        if (k == kind_) return;
        kind_ = k;
        update();
    }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        QColor fg = palette().color(QPalette::WindowText);
        if (underMouse() || isDown()) {
            QColor bg = kind_ == Close ? QColor("#e5484d") : palette().color(QPalette::AlternateBase);
            if (kind_ == Close) fg = Qt::white;
            if (isDown()) bg = bg.darker(115);
            p.setPen(Qt::NoPen);
            p.setBrush(bg);
            p.drawRoundedRect(QRectF(rect()).adjusted(3, 2, -3, -2), 8, 8);
        }
        p.setPen(QPen(fg, 1.4, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.setBrush(Qt::NoBrush);
        const QPointF c = QRectF(rect()).center();
        const double s = 4.5;
        switch (kind_) {
        case Help: {
            QFont f = font();
            f.setPixelSize(15);
            f.setBold(true);
            p.setFont(f);
            p.drawText(QRectF(rect()), Qt::AlignCenter, "?");
            break;
        }
        case Minimize: p.drawLine(QPointF(c.x() - s, c.y()), QPointF(c.x() + s, c.y())); break;
        case Maximize: p.drawRoundedRect(QRectF(c.x() - s, c.y() - s, 2 * s, 2 * s), 1.5, 1.5); break;
        case Restore: {
            p.drawRoundedRect(QRectF(c.x() - s, c.y() - s + 2.5, 2 * s - 2.5, 2 * s - 2.5), 1.5, 1.5);
            QPolygonF back;
            back << QPointF(c.x() - s + 2.5, c.y() - s + 2.5) << QPointF(c.x() - s + 2.5, c.y() - s) << QPointF(c.x() + s, c.y() - s)
                 << QPointF(c.x() + s, c.y() + s - 2.5) << QPointF(c.x() + s - 2.5, c.y() + s - 2.5);
            p.drawPolyline(back);
            break;
        }
        case Close:
            p.drawLine(QPointF(c.x() - s, c.y() - s), QPointF(c.x() + s, c.y() + s));
            p.drawLine(QPointF(c.x() + s, c.y() - s), QPointF(c.x() - s, c.y() + s));
            break;
        }
    }

private:
    Kind kind_;
};

// ------------------------------------------------------------------ logo : icône de la fenêtre, sinon le cube du cadre de chargement
class LcLogo : public QWidget {
public:
    explicit LcLogo(QWidget *parent = nullptr) : QWidget(parent) { setFixedSize(24, 24); }

protected:
    void paintEvent(QPaintEvent *) override
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::SmoothPixmapTransform);
        QIcon ic = window() ? window()->windowIcon() : QIcon();
        if (ic.isNull()) ic = QApplication::windowIcon();
        if (!ic.isNull()) {
            const QPixmap px = ic.pixmap(QSize(24, 24), devicePixelRatioF());
            if (!px.isNull()) {
                p.drawPixmap(rect(), px);
                return;
            }
        }
        const QColor accent = palette().color(QPalette::Highlight);
        const QPointF c = QRectF(rect()).center();
        const double s = 8.5, w = s * 0.866;
        auto poly = [&](QList<QPointF> pts, const QColor &col) {
            p.setPen(Qt::NoPen);
            p.setBrush(col);
            p.drawPolygon(QPolygonF(pts.toVector()));
        };
        poly({{c.x(), c.y() - s}, {c.x() + w, c.y() - s / 2}, {c.x(), c.y()}, {c.x() - w, c.y() - s / 2}}, accent.lighter(130));
        poly({{c.x() - w, c.y() - s / 2}, {c.x(), c.y()}, {c.x(), c.y() + s}, {c.x() - w, c.y() + s / 2}}, accent);
        poly({{c.x(), c.y()}, {c.x() + w, c.y() - s / 2}, {c.x() + w, c.y() + s / 2}, {c.x(), c.y() + s}}, accent.darker(135));
    }
};

// ------------------------------------------------------------------ barre de titre
class LcTitleBar : public QWidget {
public:
    LcTitleBar(QWidget *win, bool canMin, bool canMax) : QWidget(win), win_(win), canMax_(canMax)
    {
        auto *h = new QHBoxLayout(this);
        h->setContentsMargins(14, 0, 6, 0);
        h->setSpacing(10);
        logo_ = new LcLogo;
        title_ = new QLabel;
        QFont f = title_->font();
        f.setPixelSize(13);
        f.setBold(true);
        title_->setFont(f);
        title_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        h->addWidget(logo_);
        h->addWidget(title_, 1);
        if (lcHelpHook() && !win->property("lcNoHelp").toBool()) {
            auto *helpBtn = new LcCaptionButton(LcCaptionButton::Help);
            helpBtn->setToolTip(tr("Aide (F1)"));
            h->addWidget(helpBtn);
            connect(helpBtn, &QAbstractButton::clicked, win_, [this] { lcOpenHelp(win_); });
        }
        if (canMin) {
            minBtn_ = new LcCaptionButton(LcCaptionButton::Minimize);
            minBtn_->setToolTip(tr("Réduire"));
            h->addWidget(minBtn_);
            connect(minBtn_, &QAbstractButton::clicked, win_, [this] { win_->showMinimized(); });
        }
        if (canMax) {
            maxBtn_ = new LcCaptionButton(LcCaptionButton::Maximize);
            h->addWidget(maxBtn_);
            connect(maxBtn_, &QAbstractButton::clicked, this, [this] { toggleMaximize(); });
        }
        closeBtn_ = new LcCaptionButton(LcCaptionButton::Close);
        closeBtn_->setToolTip(tr("Fermer"));
        h->addWidget(closeBtn_);
        connect(closeBtn_, &QAbstractButton::clicked, win_, [this] { win_->close(); });
        setFixedHeight(40);
        refresh();
    }

    // titre, icône et bouton agrandir / restaurer d'après l'état de la fenêtre
    void refresh()
    {
        QString t = win_->windowTitle();
        t.remove("[*]");
        title_->setText(t);
        logo_->update();
        if (maxBtn_) {
            const bool max = win_->isMaximized();
            maxBtn_->setKind(max ? LcCaptionButton::Restore : LcCaptionButton::Maximize);
            maxBtn_->setToolTip(max ? tr("Restaurer") : tr("Agrandir"));
        }
    }

    void toggleMaximize()
    {
        if (!canMax_) return;
        if (win_->isMaximized()) win_->showNormal();
        else win_->showMaximized();
    }

protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton && win_->windowHandle() && !win_->isMaximized()) {
            win_->windowHandle()->startSystemMove();
            e->accept();
            return;
        }
        QWidget::mousePressEvent(e);
    }
    void mouseDoubleClickEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton) toggleMaximize();
        else QWidget::mouseDoubleClickEvent(e);
    }

private:
    QWidget *win_;
    bool canMax_;
    LcLogo *logo_;
    QLabel *title_;
    LcCaptionButton *minBtn_ = nullptr, *maxBtn_ = nullptr, *closeBtn_ = nullptr;
};

// ------------------------------------------------------------------ zone de redimensionnement sur un bord / un coin
class LcGrip : public QWidget {
public:
    LcGrip(Qt::Edges edges, QWidget *parent) : QWidget(parent), edges_(edges)
    {
        const bool l = edges.testFlag(Qt::LeftEdge), r = edges.testFlag(Qt::RightEdge);
        const bool t = edges.testFlag(Qt::TopEdge), b = edges.testFlag(Qt::BottomEdge);
        if ((l && t) || (r && b)) setCursor(Qt::SizeFDiagCursor);
        else if ((r && t) || (l && b)) setCursor(Qt::SizeBDiagCursor);
        else if (l || r) setCursor(Qt::SizeHorCursor);
        else setCursor(Qt::SizeVerCursor);
    }

protected:
    void mousePressEvent(QMouseEvent *e) override
    {
        if (e->button() == Qt::LeftButton && window()->windowHandle()) window()->windowHandle()->startSystemResize(edges_);
    }

private:
    Qt::Edges edges_;
};

// ------------------------------------------------------------------ le cadre lui-même
class LcChrome : public QObject {
public:
    // barre de titre 40 px, coins arrondis (rayon 12), bordure de 1 px
    static constexpr int kBar = 40, kRadius = 12;

    // Habille la fenêtre `win` (QMainWindow ou QDialog) ; à appeler avant de l'afficher. Sans effet si les cadres natifs sont demandés.
    static LcChrome *install(QWidget *win, bool minimize = true, bool maximize = true)
    {
        if (!win || win->property("lcFramed").toBool()) return nullptr; // déjà habillée
        installHelpShortcut(win); // F1 : aide, même avec les cadres natifs
        if (!lcCustomFrames()) return nullptr;
        return new LcChrome(win, minimize, maximize);
    }

    // F1 ouvre l'aide depuis n'importe quelle fenêtre (sauf la fenêtre d'aide elle-même)
    static void installHelpShortcut(QWidget *win)
    {
        if (win->property("lcNoHelp").toBool()) return;
        auto *sc = new QShortcut(QKeySequence(Qt::Key_F1), win);
        sc->setContext(Qt::WindowShortcut);
        QObject::connect(sc, &QShortcut::activated, win, [win] { lcOpenHelp(win); });
    }

protected:
    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (o != win_) return false;
        switch (e->type()) {
        case QEvent::Paint: paintCard(); break;
        case QEvent::Show:
        case QEvent::Resize:
            layoutChildren();
            applyMask();
            break;
        case QEvent::WindowStateChange:
            applyMargins();
            layoutChildren();
            applyMask();
            bar_->refresh();
            win_->update();
            break;
        case QEvent::WindowTitleChange:
        case QEvent::WindowIconChange: bar_->refresh(); break;
        default: break;
        }
        return false;
    }

private:
    LcChrome(QWidget *win, bool minimize, bool maximize) : QObject(win), win_(win)
    {
        win_->setWindowFlag(Qt::FramelessWindowHint, true);
        win_->setProperty("lcFramed", true);
        win_->setAttribute(Qt::WA_TranslucentBackground, false);
        bar_ = new LcTitleBar(win_, minimize, maximize);
        const Qt::Edges E[8] = {Qt::TopEdge | Qt::LeftEdge,  Qt::TopEdge | Qt::RightEdge, Qt::BottomEdge | Qt::LeftEdge,
                                Qt::BottomEdge | Qt::RightEdge, Qt::TopEdge,                Qt::BottomEdge,
                                Qt::LeftEdge,                   Qt::RightEdge};
        for (int i = 0; i < 8; ++i) grips_[i] = new LcGrip(E[i], win_);
        win_->installEventFilter(this);
        applyMargins();
        layoutChildren();
        applyMask();
        bar_->refresh();
    }

    bool full() const { return win_->windowState() & (Qt::WindowMaximized | Qt::WindowFullScreen); }

    // La carte opaque occupe toute la fenêtre.
    QRect cardRect() const { return win_->rect(); }

    // Aucun masque : les coins arrondis restent opaques.
    void applyMask()
    {
        win_->clearMask();
    }

    // le contenu de la fenêtre (menus, widget central, barre d'état, mise en page des dialogues) commence sous la barre de titre
    void applyMargins()
    {
        if (full()) win_->setContentsMargins(0, kBar, 0, 0);
        else win_->setContentsMargins(1, kBar + 1, 1, 1);
    }

    void layoutChildren()
    {
        const QRect card = cardRect();
        if (full()) bar_->setGeometry(card.left(), card.top(), card.width(), kBar);
        else bar_->setGeometry(card.left() + 1, card.top() + 1, card.width() - 2, kBar);
        const int w = win_->width(), h = win_->height(), c = 12, gl = 5, gr = 5, gt = 5, gb = 5;
        grips_[0]->setGeometry(0, 0, c, c);
        grips_[1]->setGeometry(w - c, 0, c, c);
        grips_[2]->setGeometry(0, h - c, c, c);
        grips_[3]->setGeometry(w - c, h - c, c, c);
        grips_[4]->setGeometry(c, 0, w - 2 * c, gt);
        grips_[5]->setGeometry(c, h - gb, w - 2 * c, gb);
        grips_[6]->setGeometry(0, c, gl, h - 2 * c);
        grips_[7]->setGeometry(w - gr, c, gr, h - 2 * c);
        for (LcGrip *g : grips_) {
            g->setVisible(!full());
            if (!full()) g->raise();
        }
        bar_->raise();
        for (LcGrip *g : grips_)
            if (!full()) g->raise();
    }

    // carte : fond, bandeau de la barre de titre, bordure
    void paintCard()
    {
        QPainter p(win_);
        p.setRenderHint(QPainter::Antialiasing);
        const QPalette pal = win_->palette();
        const QColor bg = pal.color(QPalette::Window), surface = pal.color(QPalette::Base), border = pal.color(QPalette::Mid);
        const bool fs = full();
        const QRectF card = QRectF(cardRect());
        const double r = fs ? 0 : kRadius;
        QPainterPath path;
        path.addRoundedRect(fs ? card : card.adjusted(0.5, 0.5, -0.5, -0.5), r, r);
        p.fillRect(card, bg);
        p.save();
        p.setClipPath(path);
        p.fillRect(QRectF(card.left(), card.top(), card.width(), kBar), surface);
        p.setPen(QPen(border, 1));
        p.drawLine(QPointF(card.left(), card.top() + kBar), QPointF(card.right(), card.top() + kBar));
        p.restore();
        if (!fs) {
            p.setPen(QPen(border, 1));
            p.setBrush(Qt::NoBrush);
            p.drawPath(path);
        }
    }

    QWidget *win_;
    LcTitleBar *bar_ = nullptr;
    LcGrip *grips_[8] = {};
};

// Boîte de dialogue avec le cadre personnalisé (sans boutons réduire / agrandir).
class LcDialog : public QDialog {
public:
    explicit LcDialog(QWidget *parent = nullptr) : QDialog(parent)
    {
        // modale pour la fenêtre parente seulement : la documentation (F1, bouton « ? ») reste utilisable pendant la boîte de dialogue
        setWindowModality(Qt::WindowModal);
        LcChrome::install(this, false, false);
    }
};

#pragma once
// Fenêtre « Documentation de LuantiCreator » : guide de l'utilisateur et guide complet pour développer des plugins LuantiCreator (manifeste, gabarits, icônes,
// bibliothèque Python luanti_creator, module luanti_creator_core). Contenu généré dans apidoc.h par gen_apidoc.py.
//
// Avec Qt WebEngine (LC_HAVE_WEBENGINE, détecté par CMake) la documentation s'affiche dans un WebView : mise en page soignée,
// coloration du code, icône « Copier » sur les blocs de code, sommaire qui suit la lecture, recherche surlignée (voir apidocview.h).
// Sans Qt WebEngine, l'ancienne fenêtre à base de QTextBrowser est utilisée (même contenu, présentation plus simple).

#include <QAbstractTextDocumentLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtWidgets>
#include <functional>

#ifdef LC_HAVE_WEBENGINE
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include "apidocview.h"
#endif

#include "apidoc.h"
#include "appcore.h"
#include "splash.h"
#include "uihooks.h"

#ifdef LC_HAVE_WEBENGINE
// Page web : les liens externes s'ouvrent dans le navigateur du système, window.prompt("lc-copy", texte) (boutons « Copier »)
// renseigne le presse-papiers sans afficher de boîte de dialogue, aucune autre navigation n'est permise.
class ApiDocPage : public QWebEnginePage {
public:
    using QWebEnginePage::QWebEnginePage;

protected:
    bool acceptNavigationRequest(const QUrl &url, NavigationType, bool) override
    {
        const QString sc = url.scheme();
        if (sc == "data" || sc == "about") return true; // chargement de la page elle-même
        if (sc == "http" || sc == "https" || sc == "mailto") QDesktopServices::openUrl(url);
        return false;
    }

    bool javaScriptPrompt(const QUrl &, const QString &msg, const QString &defaultValue, QString *result) override
    {
        if (msg != "lc-copy") return false;
        QGuiApplication::clipboard()->setText(defaultValue);
        if (result) result->clear();
        return true;
    }
};

class LcDocPane : public QWidget {
public:
    explicit LcDocPane(bool compact = false, QWidget *parent = nullptr) : QWidget(parent), compact_(compact)
    {
        auto *layout = new QVBoxLayout(this);
        layout->setContentsMargins(0, 0, 0, 0);
#ifdef LC_HAVE_WEBENGINE
        view_ = new QWebEngineView(this);
        page_ = new ApiDocPage(QWebEngineProfile::defaultProfile(), view_);
        view_->setPage(page_);
        page_->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
        page_->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);
        page_->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, false);
        page_->setBackgroundColor(QColor(themeColors().value("bg")));
        layout->addWidget(view_);
        connect(view_, &QWebEngineView::loadFinished, this, [this](bool ok) {
            if (!ok) return;
            loaded_ = true;
            pushTheme();
            if (compact_) page_->runJavaScript("lcDoc.compact();");
            render();
        });
        view_->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(view_, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
            QMenu menu(this);
            menu.addAction(tr("Copier"), [this] { page_->triggerAction(QWebEnginePage::Copy); })->setEnabled(page_->hasSelection());
            menu.addAction(tr("Tout sélectionner"), [this] { page_->triggerAction(QWebEnginePage::SelectAll); });
            menu.exec(view_->mapToGlobal(pos));
        });
        qApp->installEventFilter(this);
        view_->setHtml(lcdocview::html());
#else
        view_ = new QTextBrowser(this);
        view_->setOpenExternalLinks(true);
        view_->setReadOnly(true);
        layout->addWidget(view_);
#endif
    }

    void setMarkdown(const QString &markdown)
    {
        markdown_ = markdown;
#ifdef LC_HAVE_WEBENGINE
        render();
#else
        view_->setMarkdown(markdown_);
        view_->document()->setDocumentMargin(18);
#endif
    }

protected:
#ifdef LC_HAVE_WEBENGINE
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        if (watched == qApp && event->type() == QEvent::ApplicationPaletteChange)
            QTimer::singleShot(0, this, [this] { pushTheme(); });
        return QWidget::eventFilter(watched, event);
    }
#endif

private:
#ifdef LC_HAVE_WEBENGINE
    static LcColors themeColors()
    {
        LcColors colors = lcBaseColors(QSettings("LuantiCreator", "LuantiCreator").value("dark", true).toBool());
        for (auto it = lcOverrides().colors.begin(); it != lcOverrides().colors.end(); ++it)
            if (colors.contains(it.key()) && lcIsHexColor(it.value())) colors[it.key()] = it.value();
        return colors;
    }

    static QString jsString(const QString &text)
    {
        const QByteArray json = QJsonDocument(QJsonArray{text}).toJson(QJsonDocument::Compact);
        return QString::fromUtf8(json.mid(1, json.size() - 2));
    }

    void render()
    {
        if (loaded_) page_->runJavaScript("lcDoc.render(" + jsString(markdown_) + ");");
    }

    void pushTheme()
    {
        const LcColors colors = themeColors();
        QJsonObject json;
        for (auto it = colors.begin(); it != colors.end(); ++it) json[it.key()] = it.value();
        const bool dark = QSettings("LuantiCreator", "LuantiCreator").value("dark", true).toBool();
        page_->setBackgroundColor(QColor(colors.value("bg")));
        if (loaded_)
            page_->runJavaScript("lcDoc.setTheme(" + QString::fromUtf8(QJsonDocument(json).toJson(QJsonDocument::Compact)) + "," +
                                 (dark ? "true" : "false") + ");");
    }

    QWebEngineView *view_ = nullptr;
    ApiDocPage *page_ = nullptr;
    bool loaded_ = false;
#else
    QTextBrowser *view_ = nullptr;
#endif
    bool compact_ = false;
    QString markdown_;
};

class ApiDocWindow : public QMainWindow {
public:
    // ouvre (ou ramène au premier plan) la fenêtre de documentation ; `topic` : titre de section à afficher (facultatif)
    static void open(const QString &topic = QString())
    {
        static QPointer<ApiDocWindow> w;
        if (!w) LcSplash::busy(tr("Ouverture de la documentation"), [&] { w = new ApiDocWindow; });
        w->show();
        w->raise();
        w->activateWindow();
        if (!topic.isEmpty()) w->goTo(topic);
    }

    ApiDocWindow()
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setProperty("lcNoHelp", true); // c'est déjà l'aide
        LcChrome::install(this);
        setWindowTitle(tr("Documentation de LuantiCreator"));
        UiHooks::bindWindow(this, "apidoc");
        resize(1180, 780);

        view_ = new QWebEngineView(this);
        page_ = new ApiDocPage(QWebEngineProfile::defaultProfile(), view_);
        view_->setPage(page_);
        page_->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, false);
        page_->settings()->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, false);
        page_->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, false);
        page_->setBackgroundColor(QColor(themeColors().value("bg"))); // pas d'éclair blanc avant le premier affichage
        setCentralWidget(view_);

        // menu contextuel réduit à l'essentiel (le menu par défaut propose « Retour », « Recharger »... sans objet ici)
        view_->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(view_, &QWidget::customContextMenuRequested, this, [this](const QPoint &pos) {
            QMenu m(this);
            m.addAction(tr("Copier"), [this] { page_->triggerAction(QWebEnginePage::Copy); })->setEnabled(page_->hasSelection());
            m.addAction(tr("Tout sélectionner"), [this] { page_->triggerAction(QWebEnginePage::SelectAll); });
            m.addSeparator();
            m.addAction(tr("Rechercher…"), [this] { js("lcDoc.focusSearch()"); });
            m.exec(view_->mapToGlobal(pos));
        });

        auto *find = new QShortcut(QKeySequence::Find, this);
        connect(find, &QShortcut::activated, this, [this] { js("lcDoc.focusSearch()"); });
        auto zoom = [this](double d) { view_->setZoomFactor(d == 0 ? 1.0 : qBound(0.5, view_->zoomFactor() + d, 3.0)); };
        connect(new QShortcut(QKeySequence::ZoomIn, this), &QShortcut::activated, this, [zoom] { zoom(0.1); });
        connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Equal), this), &QShortcut::activated, this, [zoom] { zoom(0.1); });
        connect(new QShortcut(QKeySequence::ZoomOut, this), &QShortcut::activated, this, [zoom] { zoom(-0.1); });
        connect(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_0), this), &QShortcut::activated, this, [zoom] { zoom(0); });

        connect(view_, &QWebEngineView::loadFinished, this, [this](bool ok) {
            if (!ok) return;
            loaded_ = true;
            pushTheme();
            js("lcDoc.render(" + jsString(lcapidoc::markdown()) + ");");
            if (!pendingTopic_.isEmpty()) goTo(pendingTopic_);
            pendingTopic_.clear();
        });
        qApp->installEventFilter(this); // le thème (sombre / clair, plugins) suit les changements de palette
        view_->setHtml(lcdocview::html());
    }

    void goTo(const QString &title)
    {
        if (!loaded_) {
            pendingTopic_ = title;
            return;
        }
        js("lcDoc.goTo(" + jsString(title) + ");");
    }

protected:
    bool eventFilter(QObject *o, QEvent *e) override
    {
        if (o == qApp && e->type() == QEvent::ApplicationPaletteChange)
            QTimer::singleShot(0, this, [this] { pushTheme(); }); // après lcApplyTheme() (feuille de style comprise)
        return QMainWindow::eventFilter(o, e);
    }

private:
    // couleurs réellement utilisées par l'application (thème + réglages des plugins), comme lcApplyTheme()
    static LcColors themeColors()
    {
        LcColors c = lcBaseColors(QSettings("LuantiCreator", "LuantiCreator").value("dark", true).toBool());
        for (auto it = lcOverrides().colors.begin(); it != lcOverrides().colors.end(); ++it)
            if (c.contains(it.key()) && lcIsHexColor(it.value())) c[it.key()] = it.value();
        return c;
    }

    static QString jsString(const QString &s) // littéral JavaScript (JSON) pour une chaîne
    {
        const QByteArray j = QJsonDocument(QJsonArray{s}).toJson(QJsonDocument::Compact);
        return QString::fromUtf8(j.mid(1, j.size() - 2));
    }

    void js(const QString &code)
    {
        if (page_) page_->runJavaScript(code);
    }

    void pushTheme()
    {
        const LcColors c = themeColors();
        QJsonObject o;
        for (auto it = c.begin(); it != c.end(); ++it) o[it.key()] = it.value();
        const bool dark = QSettings("LuantiCreator", "LuantiCreator").value("dark", true).toBool();
        page_->setBackgroundColor(QColor(c.value("bg")));
        if (loaded_)
            js("lcDoc.setTheme(" + QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact)) + "," +
               (dark ? "true" : "false") + ");");
    }

    QWebEngineView *view_ = nullptr;
    ApiDocPage *page_ = nullptr;
    bool loaded_ = false;
    QString pendingTopic_;
};
#else
class ApiDocWindow : public QMainWindow {
public:
    // ouvre (ou ramène au premier plan) la fenêtre de documentation ; `topic` : titre de section à afficher (facultatif)
    static void open(const QString &topic = QString())
    {
        static QPointer<ApiDocWindow> w;
        if (!w) LcSplash::busy(tr("Ouverture de la documentation"), [&] { w = new ApiDocWindow; });
        w->show();
        w->raise();
        w->activateWindow();
        if (!topic.isEmpty()) w->goTo(topic);
    }

    ApiDocWindow()
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setProperty("lcNoHelp", true); // c'est déjà l'aide
        LcChrome::install(this);
        setWindowTitle(tr("Documentation de LuantiCreator"));
        UiHooks::bindWindow(this, "apidoc");
        resize(1180, 780);

        auto *central = new QWidget;
        auto *root = new QHBoxLayout(central);
        root->setContentsMargins(10, 10, 10, 10);
        auto *split = new QSplitter;
        root->addWidget(split);

        // ---- gauche : table des matières
        auto *left = new QWidget;
        auto *lv = new QVBoxLayout(left);
        lv->setContentsMargins(0, 0, 0, 0);
        filter_ = new QLineEdit;
        filter_->setPlaceholderText(tr("Filtrer les sections…"));
        filter_->setClearButtonEnabled(true);
        toc_ = new QTreeWidget;
        toc_->setHeaderHidden(true);
        toc_->setIndentation(14);
        lv->addWidget(filter_);
        lv->addWidget(toc_, 1);
        split->addWidget(left);

        // ---- droite : recherche + document
        auto *right = new QWidget;
        auto *rv = new QVBoxLayout(right);
        rv->setContentsMargins(0, 0, 0, 0);
        auto *bar = new QHBoxLayout;
        search_ = new QLineEdit;
        search_->setPlaceholderText(tr("Rechercher dans la documentation (Entrée : suivant, Maj+Entrée : précédent)"));
        search_->setClearButtonEnabled(true);
        auto *prev = new QPushButton(tr("Précédent"));
        auto *next = new QPushButton(tr("Suivant"));
        UiHooks::bind(prev, "btn.prev", "apidoc");
        UiHooks::bind(next, "btn.next", "apidoc");
        prev->setToolTip(tr("Résultat précédent"));
        next->setToolTip(tr("Résultat suivant"));
        info_ = new QLabel;
        info_->setProperty("muted", true);
        bar->addWidget(search_, 1);
        bar->addWidget(prev);
        bar->addWidget(next);
        bar->addWidget(info_);
        rv->addLayout(bar);
        view_ = new QTextBrowser;
        view_->setOpenExternalLinks(true);
        view_->setReadOnly(true);
        rv->addWidget(view_, 1);
        split->addWidget(right);
        split->setStretchFactor(0, 0);
        split->setStretchFactor(1, 1);
        split->setSizes({300, 880});
        setCentralWidget(central);

        applyDocStyle();
        view_->setMarkdown(lcapidoc::markdown());
        buildToc();

        connect(toc_, &QTreeWidget::itemClicked, this, [this](QTreeWidgetItem *it) { scrollToBlock(it->data(0, Qt::UserRole).toInt()); });
        connect(filter_, &QLineEdit::textChanged, this, [this](const QString &t) { filterToc(t); });
        connect(search_, &QLineEdit::returnPressed, this, [this] {
            findText(!(QApplication::keyboardModifiers() & Qt::ShiftModifier));
        });
        connect(search_, &QLineEdit::textChanged, this, [this] {
            view_->moveCursor(QTextCursor::Start);
            findText(true);
        });
        connect(next, &QPushButton::clicked, this, [this] { findText(true); });
        connect(prev, &QPushButton::clicked, this, [this] { findText(false); });
        auto *sc = new QShortcut(QKeySequence::Find, this);
        connect(sc, &QShortcut::activated, this, [this] {
            search_->setFocus();
            search_->selectAll();
        });
    }

    void goTo(const QString &title)
    {
        for (QTreeWidgetItemIterator it(toc_); *it; ++it)
            if ((*it)->text(0).contains(title, Qt::CaseInsensitive)) {
                toc_->setCurrentItem(*it);
                scrollToBlock((*it)->data(0, Qt::UserRole).toInt());
                return;
            }
    }

private:
    void applyDocStyle()
    {
        const bool dark = palette().color(QPalette::Window).lightness() < 128;
        const QString code = dark ? "#2f3238" : "#eceef2", line = dark ? "#3a3e46" : "#d9dce3";
        view_->document()->setDefaultStyleSheet(
            QString("h1{margin-top:26px;} h2{margin-top:20px;} h3{margin-top:14px;}"
                    "code{background:%1;} pre{background:%1;} th{background:%1;} td,th{border:1px solid %2;padding:4px;}")
                .arg(code, line));
        QFont f = view_->font();
        f.setPointSizeF(qMax(10.5, f.pointSizeF()));
        view_->setFont(f);
        view_->document()->setDocumentMargin(18);
    }

    // une entrée par titre du document (niveaux 1 à 3), imbriquées selon leur niveau
    void buildToc()
    {
        toc_->clear();
        QVector<QTreeWidgetItem *> stack(4, nullptr);
        for (QTextBlock b = view_->document()->begin(); b.isValid(); b = b.next()) {
            const int lvl = b.blockFormat().headingLevel();
            if (lvl < 1 || lvl > 3 || b.text().trimmed().isEmpty()) continue;
            auto *it = new QTreeWidgetItem;
            it->setText(0, b.text().trimmed());
            it->setData(0, Qt::UserRole, b.blockNumber());
            QTreeWidgetItem *parent = nullptr;
            for (int l = lvl - 1; l >= 1 && !parent; --l) parent = stack[l];
            if (parent) parent->addChild(it);
            else toc_->addTopLevelItem(it);
            stack[lvl] = it;
            for (int l = lvl + 1; l <= 3; ++l) stack[l] = nullptr;
        }
        for (int i = 0; i < toc_->topLevelItemCount(); ++i) toc_->topLevelItem(i)->setExpanded(true);
    }

    void filterToc(const QString &t)
    {
        for (QTreeWidgetItemIterator it(toc_); *it; ++it) (*it)->setHidden(false);
        if (t.trimmed().isEmpty()) return;
        // masque les feuilles qui ne correspondent pas, garde les parents dont un descendant correspond
        std::function<bool(QTreeWidgetItem *)> visit = [&](QTreeWidgetItem *n) {
            bool any = n->text(0).contains(t, Qt::CaseInsensitive);
            for (int i = 0; i < n->childCount(); ++i) any |= visit(n->child(i));
            n->setHidden(!any);
            if (any) n->setExpanded(true);
            return any;
        };
        for (int i = 0; i < toc_->topLevelItemCount(); ++i) visit(toc_->topLevelItem(i));
    }

    void scrollToBlock(int blockNumber)
    {
        const QTextBlock b = view_->document()->findBlockByNumber(blockNumber);
        if (!b.isValid()) return;
        const QRectF r = view_->document()->documentLayout()->blockBoundingRect(b);
        view_->verticalScrollBar()->setValue(int(r.top()) - 6);
    }

    void findText(bool forward)
    {
        const QString t = search_->text();
        if (t.isEmpty()) {
            info_->clear();
            return;
        }
        QTextDocument::FindFlags fl;
        if (!forward) fl |= QTextDocument::FindBackward;
        bool ok = view_->find(t, fl);
        if (!ok) { // reboucle
            view_->moveCursor(forward ? QTextCursor::Start : QTextCursor::End);
            ok = view_->find(t, fl);
        }
        info_->setText(ok ? QString() : tr("Aucun résultat"));
    }

    QLineEdit *filter_ = nullptr, *search_ = nullptr;
    QTreeWidget *toc_ = nullptr;
    QTextBrowser *view_ = nullptr;
    QLabel *info_ = nullptr;
};
#endif // LC_HAVE_WEBENGINE

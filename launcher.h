#pragma once
// Fenêtres « hors édition d'un mod » :
//   LauncherWindow       : fenêtre d'accueil de LuantiCreator — projets (nouveau, ouvrir, importer, dupliquer,
//                          exporter, supprimer), gestion des plugins, gestion de LuantiCreator (thème, dossiers...).
//                          Ouvrir un mod ferme cette fenêtre et ouvre la fenêtre d'édition du mod.
//   PluginEditorWindow   : édition d'UN plugin, dans sa propre fenêtre (fichiers, textures, blocs, icônes).
//   PluginManagerWindow  : le gestionnaire de plugins seul (accessible depuis l'éditeur d'un mod).

#include <QtWidgets>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <functional>

#include "apidocwindow.h"
#include "appcore.h"
#include "fsutil.h"
#include "generator.h"
#include "modimport.h"
#include "pluginpage.h"
#include "plugins.h"
#include "splash.h"

#ifndef LC_VERSION
#define LC_VERSION "0.9"
#endif

// ------------------------------------------------------------------ éditeur d'un plugin (fenêtre séparée)

class PluginEditorWindow : public QMainWindow {
public:
    // ouvre (ou ramène au premier plan) la fenêtre d'édition du plugin
    static void open(const QString &pluginId)
    {
        static QMap<QString, QPointer<PluginEditorWindow>> wins;
        if (!plugins().find(pluginId)) {
            QMessageBox::warning(nullptr, tr("Plugin"), tr("Plugin introuvable : %1").arg(pluginId));
            return;
        }
        QPointer<PluginEditorWindow> w = wins.value(pluginId);
        if (!w) {
            LcSplash::busy(tr("Ouverture de l'éditeur de plugin"), [&] { w = new PluginEditorWindow(pluginId); },
                           plugins().find(pluginId)->name);
            wins[pluginId] = w;
        }
        w->show();
        w->raise();
        w->activateWindow();
    }

    explicit PluginEditorWindow(const QString &id) : id_(id)
    {
        setAttribute(Qt::WA_DeleteOnClose);
        LcChrome::install(this);
        resize(1060, 740);
        UiHooks::bindWindow(this, "pluginedit");
        page_ = new PluginPage(PluginPageMode::Editor);
        setCentralWidget(page_);
        page_->setPlugin(id);
        auto *fm = menuBar()->addMenu(tr("&Fichier"));
        auto *save = fm->addAction(tr("&Enregistrer"), [this] { page_->saveCurrent(); });
        UiHooks::bind(save, "menu.save", "pluginedit");
        save->setShortcut(QKeySequence::Save);
        auto *closeAct = fm->addAction(tr("&Fermer la fenêtre"), [this] { this->close(); });
        closeAct->setShortcut(QKeySequence("Ctrl+W"));
        UiHooks::bind(closeAct, "menu.close", "pluginedit");
        auto *hm = menuBar()->addMenu(tr("&Aide"));
        auto *docAct = hm->addAction(tr("&Documentation de LuantiCreator…"), [] { ApiDocWindow::open(); });
        UiHooks::bind(docAct, "menu.apidoc", "pluginedit");
        page_->onChanged = [this] {
            UiHooks::refresh(); // icônes que le plugin édité vient de déclarer
            updateTitle();
        };
        updateTitle();
        // le plugin a été supprimé ou remplacé depuis le gestionnaire
        AppBus::subscribe(this, "plugins", [this] {
            if (!plugins().find(id_)) {
                closing_ = true; // plus rien à enregistrer
                close();
                return;
            }
            UiHooks::refresh();
            updateTitle();
            if (!page_->isDirty()) page_->rebuild();
        });
    }

protected:
    void closeEvent(QCloseEvent *e) override
    {
        if (!closing_ && page_->isDirty()) {
            const auto r = QMessageBox::question(this, tr("Plugin"),
                                                 tr("Le fichier ouvert a des modifications non enregistrées. L'enregistrer ?"),
                                                 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Save);
            if (r == QMessageBox::Cancel || (r == QMessageBox::Save && !page_->saveCurrent())) {
                e->ignore();
                return;
            }
        }
        QMainWindow::closeEvent(e);
    }

private:
    void updateTitle()
    {
        const Plugin *p = plugins().find(id_);
        setWindowTitle(tr("Éditeur de plugin — %1").arg(p ? p->name : id_));
    }

    QString id_;
    PluginPage *page_ = nullptr;
    bool closing_ = false;
};

// ------------------------------------------------------------------ gestionnaire de plugins seul

class PluginManagerWindow : public QMainWindow {
public:
    static void open()
    {
        static QPointer<PluginManagerWindow> w;
        if (!w) LcSplash::busy(tr("Ouverture du gestionnaire de plugins"), [&] { w = new PluginManagerWindow; });
        w->show();
        w->raise();
        w->activateWindow();
    }

    PluginManagerWindow()
    {
        setAttribute(Qt::WA_DeleteOnClose);
        LcChrome::install(this);
        setWindowTitle(tr("Gestionnaire de plugins"));
        UiHooks::bindWindow(this, "plugins");
        resize(980, 640);
        page_ = new PluginPage(PluginPageMode::Manage);
        page_->editPlugin = [](const QString &id) { PluginEditorWindow::open(id); };
        setCentralWidget(page_);
        page_->onChanged = [] { UiHooks::refresh(); };
        AppBus::subscribe(page_, "plugins", [this] {
            UiHooks::refresh();
            page_->rebuild();
        });
    }

private:
    PluginPage *page_;
};

// ------------------------------------------------------------------ lanceur

class LauncherWindow : public QMainWindow {
public:
    LauncherWindow()
    {
        setAttribute(Qt::WA_DeleteOnClose);
        setProperty("lcHelpTopic", "Le lanceur");
        LcChrome::install(this);
        setWindowTitle(tr("LuantiCreator — Projets"));
        resize(1100, 700);
        restoreGeometry(QSettings("LuantiCreator", "LuantiCreator").value("launcherGeometry").toByteArray()); // taille et place mémorisées
        UiHooks::bindWindow(this, "launcher");

        auto *central = new QWidget;
        auto *root = new QVBoxLayout(central);
        auto *title = new QLabel("LuantiCreator");
        title->setProperty("role", "title");
        auto *sub = new QLabel(tr("Créez des mods Luanti sans écrire de Lua — version %1").arg(LC_VERSION));
        sub->setProperty("muted", true);
        root->addWidget(title);
        root->addWidget(sub);
        tabs_ = new QTabWidget;
        root->addWidget(tabs_, 1);
        setCentralWidget(central);

        tabs_->addTab(buildProjectsTab(), tr("Projets"));
        tabs_->addTab(buildPluginsTab(), tr("Plugins"));
        tabs_->addTab(buildSettingsTab(), tr("LuantiCreator"));
        applyTabIcons();

        refreshProjects();
        refreshSettings();
        if (!startupNote.isEmpty()) statusBar()->showMessage(startupNote, 8000);
        else statusBar()->showMessage(tr("Choisissez un mod à ouvrir, ou créez-en un."));

        AppBus::subscribe(this, "plugins", [this] {
            UiHooks::refresh();
            applyTabIcons();
            refreshSettings();
        });
    }

    int currentTab() const { return tabs_ ? tabs_->currentIndex() : 0; }
    void setCurrentTab(int i)
    {
        if (tabs_ && i >= 0 && i < tabs_->count()) tabs_->setCurrentIndex(i);
    }

    // message affiché au démarrage (plugins intégrés installés...)
    static inline QString startupNote;

    void closeEvent(QCloseEvent *e) override
    {
        QSettings("LuantiCreator", "LuantiCreator").setValue("launcherGeometry", saveGeometry());
        QMainWindow::closeEvent(e);
    }

private:
    // ============================================================ projets
    struct Proj {
        QString id, path, title, desc, deps;
        int elements = 0;
        QDateTime modified;
        QMap<QString, int> counts;
    };

    static QString kindLabel(const QString &key)
    {
        static const QMap<QString, QString> m = {
            {"nodes", QObject::tr("bloc(s)")}, {"models", QObject::tr("bloc(s) 3D")}, {"items", QObject::tr("objet(s)")},
            {"foods", QObject::tr("nourriture(s)")}, {"tools", QObject::tr("outil(s)")}, {"crafts", QObject::tr("recette(s)")},
            {"ores", QObject::tr("minerai(s)")}, {"biomes", QObject::tr("biome(s)")}, {"decos", QObject::tr("décoration(s)")},
            {"entities", QObject::tr("entité(s)")}, {"cmds", QObject::tr("commande(s)")}, {"formspecs", QObject::tr("formulaire(s)")}};
        return m.value(key, key);
    }

    static QVector<Proj> readProjects()
    {
        QVector<Proj> v;
        const QFileInfoList files = QDir(lcProjectsDir()).entryInfoList(QStringList() << "*.lcproj", QDir::Files, QDir::Time);
        static const QStringList keys = {"nodes", "models", "items", "foods", "tools", "crafts", "ores", "biomes", "decos",
                                         "entities", "cmds", "formspecs"};
        for (const QFileInfo &fi : files) {
            QFile f(fi.absoluteFilePath());
            if (!f.open(QIODevice::ReadOnly)) continue;
            const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
            if (o.isEmpty()) continue;
            Proj p;
            p.id = fi.completeBaseName();
            p.path = fi.absoluteFilePath();
            p.title = o["title"].toString();
            p.desc = o["desc"].toString();
            p.deps = o["deps"].toString();
            p.modified = fi.lastModified();
            for (const QString &k : keys) {
                const int n = o[k].toArray().size();
                if (n) p.counts[k] = n;
                p.elements += n;
            }
            const QJsonObject fo = o["features"].toObject();
            for (auto it = fo.begin(); it != fo.end(); ++it) p.elements += it.value().toArray().size();
            v.append(p);
        }
        return v;
    }

    QWidget *buildProjectsTab()
    {
        auto *w = new QWidget;
        auto *h = new QHBoxLayout(w);

        auto *left = new QVBoxLayout;
        search_ = new QLineEdit;
        search_->setPlaceholderText(tr("Rechercher un projet..."));
        search_->setClearButtonEnabled(true);
        projList_ = new QListWidget;
        projList_->setIconSize(QSize(32, 32));
        projList_->setSpacing(2);
        left->addWidget(search_);
        left->addWidget(projList_, 1);

        auto *right = new QVBoxLayout;
        auto *top = new QHBoxLayout;
        auto *bNew = new QPushButton(tr("Nouveau mod..."));
        UiHooks::bind(bNew, "btn.new", "launcher");
        bNew->setMinimumHeight(36);
        bNew->setProperty("primary", true);
        auto *bImport = new QToolButton;
        bImport->setText(tr("Importer un mod"));
        bImport->setPopupMode(QToolButton::InstantPopup);
        bImport->setMinimumHeight(36);
        UiHooks::bind(bImport, "btn.import", "launcher");
        auto *imenu = new QMenu(bImport);
        UiHooks::bind(imenu->addAction(tr("Un mod Luanti (.zip)..."), [this] { importModZip(); }), "menu.import.zip", "launcher");
        UiHooks::bind(imenu->addAction(tr("Un dossier de mod..."), [this] { importModFolder(); }), "menu.import.folder", "launcher");
        imenu->addSeparator();
        UiHooks::bind(imenu->addAction(tr("Un projet LuantiCreator (.lcproj)..."), [this] { importProjectFile(); }), "menu.import.project", "launcher");
        bImport->setMenu(imenu);
        top->addWidget(bNew);
        top->addWidget(bImport);
        top->addStretch(1);
        right->addLayout(top);

        info_ = new QTextBrowser;
        info_->setOpenLinks(false);
        right->addWidget(info_, 1);

        auto *bar = new QHBoxLayout;
        bOpen_ = new QPushButton(tr("Ouvrir"));
        bOpen_->setMinimumHeight(34);
        bOpen_->setProperty("primary", true);
        bDup_ = new QPushButton(tr("Dupliquer..."));
        bZip_ = new QPushButton(tr("Exporter en .zip..."));
        bFolder_ = new QPushButton(tr("Dossier du mod"));
        bDel_ = new QPushButton(tr("Supprimer"));
        for (QPushButton *b : {bOpen_, bDup_, bZip_, bFolder_, bDel_}) bar->addWidget(b);
        UiHooks::bind(bOpen_, "btn.open", "launcher");
        UiHooks::bind(bDup_, "btn.dup", "launcher");
        UiHooks::bind(bZip_, "btn.export", "launcher");
        UiHooks::bind(bFolder_, "btn.folder", "launcher");
        UiHooks::bind(bDel_, "btn.del", "launcher");
        right->addLayout(bar);

        h->addLayout(left, 2);
        h->addLayout(right, 3);

        connect(bNew, &QPushButton::clicked, this, [this] { newProject(); });
        connect(bOpen_, &QPushButton::clicked, this, [this] { openCurrent(); });
        connect(bDup_, &QPushButton::clicked, this, [this] { duplicateCurrent(); });
        connect(bZip_, &QPushButton::clicked, this, [this] { exportCurrent(); });
        connect(bFolder_, &QPushButton::clicked, this, [this] { openCurrentFolder(); });
        connect(bDel_, &QPushButton::clicked, this, [this] { deleteCurrent(); });
        connect(projList_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *) { openCurrent(); });
        connect(projList_, &QListWidget::currentRowChanged, this, [this](int) { showProjectInfo(); });
        connect(search_, &QLineEdit::textChanged, this, [this](const QString &) { refreshProjects(); });

        // ergonomie : infobulles et raccourcis (Entrée ouvre, Suppr supprime, Ctrl+N nouveau, Ctrl+F recherche)
        bNew->setToolTip(tr("Créer un nouveau mod (Ctrl+N)"));
        bOpen_->setToolTip(tr("Ouvrir le mod sélectionné (Entrée ou double-clic)"));
        bDel_->setToolTip(tr("Supprimer le mod sélectionné (Suppr)"));
        search_->setToolTip(tr("Ctrl+F — Entrée ouvre le premier résultat"));
        auto addSc = [this](const QKeySequence &k, QWidget *ctx, Qt::ShortcutContext c, std::function<void()> fn) {
            auto *s = new QShortcut(k, ctx);
            s->setContext(c);
            connect(s, &QShortcut::activated, this, [fn] { fn(); });
        };
        addSc(QKeySequence("Ctrl+N"), w, Qt::WidgetWithChildrenShortcut, [this] { newProject(); });
        addSc(QKeySequence("Ctrl+F"), w, Qt::WidgetWithChildrenShortcut, [this] {
            search_->setFocus();
            search_->selectAll();
        });
        addSc(QKeySequence(Qt::Key_Return), projList_, Qt::WidgetShortcut, [this] { openCurrent(); });
        addSc(QKeySequence(Qt::Key_Enter), projList_, Qt::WidgetShortcut, [this] { openCurrent(); });
        addSc(QKeySequence(Qt::Key_Delete), projList_, Qt::WidgetShortcut, [this] { deleteCurrent(); });
        connect(search_, &QLineEdit::returnPressed, this, [this] {
            if (!currentId().isEmpty()) openCurrent();
        });
        return w;
    }

    QString currentId() const
    {
        auto *it = projList_ ? projList_->currentItem() : nullptr;
        return it ? it->data(Qt::UserRole).toString() : QString();
    }

    void refreshProjects(const QString &select = QString())
    {
        const QString keep = select.isEmpty() ? currentId() : select;
        const QString last = QFileInfo(QSettings("LuantiCreator", "LuantiCreator").value("lastProject").toString()).completeBaseName();
        projects_ = readProjects();
        const QString q = search_ ? search_->text().trimmed().toLower() : QString();
        projList_->blockSignals(true);
        projList_->clear();
        for (const Proj &p : projects_) {
            if (!q.isEmpty() && !p.id.toLower().contains(q) && !p.title.toLower().contains(q)) continue;
            QString txt = (p.title.isEmpty() ? p.id : p.title) + "\n" + p.id + "  ·  " +
                          tr("%1 élément(s)").arg(p.elements) + "  ·  " + QLocale().toString(p.modified, QLocale::ShortFormat);
            if (p.id == last) txt += "  ·  " + tr("dernier ouvert");
            auto *it = new QListWidgetItem(txt);
            it->setData(Qt::UserRole, p.id);
            projList_->addItem(it);
        }
        projList_->blockSignals(false);
        int row = -1;
        for (int i = 0; i < projList_->count(); ++i)
            if (projList_->item(i)->data(Qt::UserRole).toString() == keep) row = i;
        if (row < 0 && projList_->count()) row = 0;
        projList_->setCurrentRow(row);
        showProjectInfo();
    }

    const Proj *projectById(const QString &id) const
    {
        for (const Proj &p : projects_)
            if (p.id == id) return &p;
        return nullptr;
    }

    void showProjectInfo()
    {
        const Proj *p = projectById(currentId());
        for (QPushButton *b : {bOpen_, bDup_, bZip_, bFolder_, bDel_}) b->setEnabled(p != nullptr);
        if (!p) {
            info_->setHtml(tr("<h3>Aucun projet</h3><p>Créez votre premier mod avec <b>Nouveau mod</b>, ou importez un mod existant "
                              "(archive .zip ou dossier) avec <b>Importer un mod</b>.</p>"));
            return;
        }
        QString h = QString("<h2>%1</h2><p><code>%2</code></p>").arg((p->title.isEmpty() ? p->id : p->title).toHtmlEscaped(), p->id.toHtmlEscaped());
        if (!p->desc.isEmpty()) h += "<p>" + p->desc.toHtmlEscaped() + "</p>";
        h += tr("<p>Dépendances : %1</p>").arg(p->deps.isEmpty() ? tr("aucune") : p->deps.toHtmlEscaped());
        if (p->counts.isEmpty()) h += tr("<p><i>Projet vide.</i></p>");
        else {
            h += "<ul>";
            for (auto it = p->counts.begin(); it != p->counts.end(); ++it)
                h += QString("<li>%1 %2</li>").arg(it.value()).arg(kindLabel(it.key()).toHtmlEscaped());
            h += "</ul>";
        }
        h += tr("<p><small>Modifié le %1<br>%2</small></p>")
                 .arg(QLocale().toString(p->modified, QLocale::LongFormat).toHtmlEscaped(), p->path.toHtmlEscaped());
        info_->setHtml(h);
    }

    // ------------------------------------------------------------ actions sur les projets
    void openProjectPath(const QString &path)
    {
        if (!AppFlow::openProject) return;
        AppFlow::openProject(path); // ouvre la fenêtre d'édition...
        close();                    // ...puis ferme cette fenêtre
    }

    void openCurrent()
    {
        const Proj *p = projectById(currentId());
        if (p) openProjectPath(p->path);
    }

    // demande identifiant / titre / description / dépendances ; false si annulé
    bool askNewProject(QString &id, QString &title, QString &desc, QString &deps)
    {
        LcDialog dlg(this);
        dlg.setWindowTitle(tr("Nouveau mod"));
        UiHooks::bindWindow(&dlg, "newmod");
        dlg.setMinimumWidth(460);
        auto *f = new QFormLayout(&dlg);
        auto *idEdit = new QLineEdit(lcimport::uniqueProjectId("mon_mod"));
        auto *titleEdit = new QLineEdit(tr("Mon mod"));
        auto *descEdit = new QLineEdit;
        auto *depsEdit = new QLineEdit("default");
        depsEdit->setPlaceholderText(tr("ex : default, farming"));
        f->addRow(tr("Identifiant (a-z, 0-9, _)"), idEdit);
        f->addRow(tr("Titre"), titleEdit);
        f->addRow(tr("Description"), descEdit);
        f->addRow(tr("Dépendances"), depsEdit);
        auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        UiHooks::bindBox(bb, "newmod");
        f->addRow(bb);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        connect(bb, &QDialogButtonBox::accepted, &dlg, [&] {
            const QString v = identOf(idEdit->text());
            if (v.isEmpty()) {
                QMessageBox::warning(&dlg, tr("Nouveau mod"), tr("Donnez un identifiant au mod."));
                return;
            }
            if (QFile::exists(lcProjectFile(v))) {
                QMessageBox::warning(&dlg, tr("Nouveau mod"), tr("Un projet « %1 » existe déjà : choisissez un autre identifiant.").arg(v));
                return;
            }
            dlg.accept();
        });
        if (dlg.exec() != QDialog::Accepted) return false;
        id = identOf(idEdit->text());
        title = titleEdit->text().trimmed();
        desc = descEdit->text().trimmed();
        deps = depsEdit->text().trimmed();
        return true;
    }

    void newProject()
    {
        QString id, title, desc, deps;
        if (!askNewProject(id, title, desc, deps)) return;
        QJsonObject o;
        o["name"] = id;
        o["title"] = title.isEmpty() ? id : title;
        o["desc"] = desc;
        o["deps"] = deps;
        QDir().mkpath(lcProjectsDir());
        QFile f(lcProjectFile(id));
        if (!f.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, tr("Nouveau mod"), tr("Impossible d'écrire %1").arg(lcProjectFile(id)));
            return;
        }
        f.write(QJsonDocument(o).toJson());
        f.close();
        openProjectPath(lcProjectFile(id));
    }

    void duplicateCurrent()
    {
        const Proj *p = projectById(currentId());
        if (!p) return;
        bool ok = false;
        const QString nid = identOf(QInputDialog::getText(this, tr("Dupliquer"), tr("Identifiant de la copie :"), QLineEdit::Normal,
                                                          lcimport::uniqueProjectId(p->id + "_copie"), &ok));
        if (!ok || nid.isEmpty()) return;
        if (QFile::exists(lcProjectFile(nid))) {
            QMessageBox::warning(this, tr("Dupliquer"), tr("Un projet « %1 » existe déjà.").arg(nid));
            return;
        }
        QFile f(p->path);
        if (!f.open(QIODevice::ReadOnly)) return;
        QString text = QString::fromUtf8(f.readAll());
        f.close();
        // les textures importées sont copiées avec le projet et ses chemins sont réécrits
        const QString oldImp = lcImportsDir() + "/" + p->id, newImp = lcImportsDir() + "/" + nid;
        if (QDir(oldImp).exists()) {
            QString err;
            QDir(newImp).removeRecursively();
            if (copyDir(oldImp, newImp, &err)) text.replace(oldImp + "/", newImp + "/");
        }
        QJsonObject o = QJsonDocument::fromJson(text.toUtf8()).object();
        o["name"] = nid;
        o["title"] = (p->title.isEmpty() ? p->id : p->title) + tr(" (copie)");
        QFile w(lcProjectFile(nid));
        if (!w.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, tr("Dupliquer"), tr("Impossible d'écrire %1").arg(lcProjectFile(nid)));
            return;
        }
        w.write(QJsonDocument(o).toJson());
        w.close();
        refreshProjects(nid);
        statusBar()->showMessage(tr("Projet dupliqué : %1").arg(nid), 5000);
    }

    void exportCurrent()
    {
        const Proj *p = projectById(currentId());
        if (!p) return;
        const QString src = lcModsWorkspace() + "/" + p->id;
        if (!QDir(src).exists()) {
            QMessageBox::information(this, tr("Exporter"),
                                     tr("Le dossier de ce mod n'a pas encore été généré : ouvrez le projet une fois, puis exportez-le."));
            return;
        }
        const QString zp = QFileDialog::getSaveFileName(this, tr("Archive du mod"), QDir::homePath() + "/" + p->id + ".zip",
                                                        tr("Archive zip (*.zip)"));
        if (zp.isEmpty()) return;
        QString err;
        if (writeZip(zp, src, p->id, &err)) QMessageBox::information(this, tr("Exporter"), tr("Archive créée : %1").arg(zp));
        else QMessageBox::warning(this, tr("Exporter"), err);
    }

    void openCurrentFolder()
    {
        const Proj *p = projectById(currentId());
        if (!p) return;
        const QString d = lcModsWorkspace() + "/" + p->id;
        if (!QDir(d).exists()) {
            QMessageBox::information(this, tr("Dossier du mod"),
                                     tr("Le dossier de ce mod n'a pas encore été généré : ouvrez le projet une fois."));
            return;
        }
        QDesktopServices::openUrl(QUrl::fromLocalFile(d));
    }

    void deleteCurrent()
    {
        const Proj *p = projectById(currentId());
        if (!p) return;
        const QString id = p->id;
        if (QMessageBox::warning(this, tr("Supprimer"),
                                 tr("Supprimer le projet « %1 » ?\n\nLe projet, son dossier de mod généré et ses textures importées seront "
                                    "effacés. Les mods déjà installés dans Luanti ou exportés en .zip ne sont pas touchés.").arg(id),
                                 QMessageBox::Yes | QMessageBox::Cancel, QMessageBox::Cancel) != QMessageBox::Yes)
            return;
        QFile::remove(lcProjectFile(id));
        QDir(lcModsWorkspace() + "/" + id).removeRecursively();
        QDir(lcImportsDir() + "/" + id).removeRecursively();
        QSettings st("LuantiCreator", "LuantiCreator");
        if (QFileInfo(st.value("lastProject").toString()).completeBaseName() == id) st.remove("lastProject");
        refreshProjects();
        statusBar()->showMessage(tr("Projet supprimé : %1").arg(id), 5000);
    }

    // ------------------------------------------------------------ importer
    void afterImport(const lcimport::Result &r)
    {
        if (!r.ok) {
            if (!r.error.isEmpty()) QMessageBox::warning(this, tr("Importer un mod"), r.error);
            return;
        }
        refreshProjects(r.id);
        const auto a = QMessageBox::question(this, tr("Importer un mod"), r.summary + "\n\n" + tr("Ouvrir le projet maintenant ?"),
                                             QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes);
        if (a == QMessageBox::Yes) openProjectPath(r.projectPath);
    }

    void importModZip()
    {
        const QString zip = QFileDialog::getOpenFileName(this, tr("Mod Luanti (.zip)"), QDir::homePath(),
                                                         tr("Archive zip (*.zip);;Tous les fichiers (*)"));
        if (zip.isEmpty()) return;
        QApplication::setOverrideCursor(Qt::WaitCursor);
        const lcimport::Result r = lcimport::importZip(this, zip);
        QApplication::restoreOverrideCursor();
        afterImport(r);
    }

    void importModFolder()
    {
        const QString d = QFileDialog::getExistingDirectory(this, tr("Dossier d'un mod Luanti (contenant mod.conf ou init.lua)"),
                                                            QDir::homePath());
        if (d.isEmpty()) return;
        QApplication::setOverrideCursor(Qt::WaitCursor);
        const lcimport::Result r = lcimport::importFolder(d);
        QApplication::restoreOverrideCursor();
        afterImport(r);
    }

    void importProjectFile()
    {
        const QString src = QFileDialog::getOpenFileName(this, tr("Projet LuantiCreator"), QDir::homePath(),
                                                         tr("Projet LuantiCreator (*.lcproj)"));
        if (src.isEmpty()) return;
        QFile f(src);
        if (!f.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, tr("Importer"), tr("Impossible de lire %1").arg(src));
            return;
        }
        QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
        if (o.isEmpty()) {
            QMessageBox::warning(this, tr("Importer"), tr("Ce fichier n'est pas un projet LuantiCreator valide."));
            return;
        }
        QString id = identOf(o["name"].toString());
        if (id.isEmpty()) id = identOf(QFileInfo(src).completeBaseName());
        if (id.isEmpty()) id = "mon_mod";
        if (QFile::exists(lcProjectFile(id))) {
            QMessageBox box(QMessageBox::Question, tr("Importer"),
                            tr("Un projet « %1 » existe déjà.").arg(id), QMessageBox::NoButton, this);
            auto *bRep = box.addButton(tr("Remplacer"), QMessageBox::DestructiveRole);
            auto *bNew = box.addButton(tr("Garder les deux"), QMessageBox::AcceptRole);
            box.addButton(QMessageBox::Cancel);
            box.exec();
            if (box.clickedButton() == bNew) id = lcimport::uniqueProjectId(id);
            else if (box.clickedButton() != bRep) return;
        }
        o["name"] = id;
        QDir().mkpath(lcProjectsDir());
        QFile w(lcProjectFile(id));
        if (!w.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, tr("Importer"), tr("Impossible d'écrire %1").arg(lcProjectFile(id)));
            return;
        }
        w.write(QJsonDocument(o).toJson());
        w.close();
        refreshProjects(id);
        statusBar()->showMessage(tr("Projet importé : %1").arg(id), 5000);
    }

    // ============================================================ plugins
    QWidget *buildPluginsTab()
    {
        pluginPage_ = new PluginPage(PluginPageMode::Manage);
        pluginPage_->editPlugin = [](const QString &id) { PluginEditorWindow::open(id); }; // fenêtre séparée
        pluginPage_->onChanged = [this] { // le gestionnaire de cet onglet a modifié les plugins
            UiHooks::refresh();
            applyTabIcons();
            refreshSettings();
        };
        AppBus::subscribe(pluginPage_, "plugins", [this] { pluginPage_->rebuild(); });
        // barre du haut : accès à la documentation complète de l'API de développement
        auto *w = new QWidget;
        auto *v = new QVBoxLayout(w);
        v->setContentsMargins(0, 0, 0, 0);
        auto *bar = new QHBoxLayout;
        auto *hint = new QLabel(tr("Besoin d'aide ? Le guide complet de LuantiCreator, l'API des plugins et un guide pas à pas sont dans la documentation."));
        hint->setProperty("muted", true);
        auto *bDoc = new QPushButton(tr("Documentation"));
        bDoc->setToolTip(tr("Ouvre la documentation : guide de l'utilisateur, plugins, manifeste, gabarits, icônes, bibliothèques Python luanti_creator et luanti_creator_core"));
        bDoc->setCursor(Qt::PointingHandCursor);
        UiHooks::bind(bDoc, "btn.apidoc", "launcher");
        connect(bDoc, &QPushButton::clicked, this, [] { ApiDocWindow::open(); });
        bar->addWidget(hint, 1);
        bar->addWidget(bDoc);
        v->addLayout(bar);
        v->addWidget(pluginPage_, 1);
        return w;
    }

    // ============================================================ gestion de LuantiCreator
    QWidget *buildSettingsTab()
    {
        auto *w = new QWidget;
        auto *v = new QVBoxLayout(w);

        auto *look = new QGroupBox(tr("Apparence"));
        auto *lv = new QVBoxLayout(look);
        darkBox_ = new QCheckBox(tr("Thème sombre"));
        lv->addWidget(darkBox_);
        v->addWidget(look);

        auto *dirs = new QGroupBox(tr("Dossiers de LuantiCreator"));
        auto *dg = new QGridLayout(dirs);
        struct D { QString label, path; };
        const QList<D> list = {{tr("Données"), lcAppDir()},
                               {tr("Projets"), lcProjectsDir()},
                               {tr("Mods générés"), lcModsWorkspace()},
                               {tr("Textures importées"), lcImportsDir()},
                               {tr("Plugins"), PluginHost::root()}};
        int row = 0;
        for (const D &d : list) {
            auto *name = new QLabel(d.label);
            auto *path = new QLabel(d.path);
            path->setTextInteractionFlags(Qt::TextSelectableByMouse);
            path->setProperty("muted", true);
            auto *btn = new QPushButton(tr("Ouvrir"));
            UiHooks::bind(btn, "btn.folder", "launcher");
            const QString p = d.path;
            connect(btn, &QPushButton::clicked, this, [p] {
                QDir().mkpath(p);
                QDesktopServices::openUrl(QUrl::fromLocalFile(p));
            });
            dg->addWidget(name, row, 0);
            dg->addWidget(path, row, 1);
            dg->addWidget(btn, row, 2);
            ++row;
        }
        dg->setColumnStretch(1, 1);
        v->addWidget(dirs);

        auto *state = new QGroupBox(tr("État"));
        auto *sv = new QVBoxLayout(state);
        stateLbl_ = new QLabel;
        stateLbl_->setWordWrap(true);
        coreLbl_ = new QLabel;
        coreLbl_->setWordWrap(true);
        coreLbl_->setProperty("role", "warning");
        sv->addWidget(stateLbl_);
        sv->addWidget(coreLbl_);
        v->addWidget(state);

        auto *maint = new QGroupBox(tr("Maintenance"));
        auto *mv = new QHBoxLayout(maint);
        auto *bReload = new QPushButton(tr("Recharger les plugins"));
        auto *bReset = new QPushButton(tr("Réinitialiser les réglages..."));
        bReset->setToolTip(tr("Remet le thème, les dossiers d'export mémorisés et le dernier projet à zéro. Vos projets et plugins ne sont pas touchés."));
        auto *bAbout = new QPushButton(tr("À propos"));
        UiHooks::bind(bReload, "btn.reload", "launcher");
        UiHooks::bind(bReset, "btn.reset", "launcher");
        UiHooks::bind(bAbout, "btn.about", "launcher");
        mv->addWidget(bReload);
        mv->addWidget(bReset);
        mv->addWidget(bAbout);
        mv->addStretch(1);
        v->addWidget(maint);
        v->addStretch(1);

        {
            QSettings st("LuantiCreator", "LuantiCreator");
            darkBox_->setChecked(st.value("dark", true).toBool());
        }
        connect(darkBox_, &QCheckBox::toggled, this, [](bool on) {
            QSettings st("LuantiCreator", "LuantiCreator");
            st.setValue("dark", on);
            lcApplyTheme(on);
        });
        connect(bReload, &QPushButton::clicked, this, [this] {
            plugins().reload();
            if (AppFlow::reloadApp) { // recharge toute l'application (cette fenêtre est recréée)
                AppFlow::reloadApp();
                return;
            }
            pluginPage_->rebuild();
            AppBus::notify(this, "plugins");
            refreshSettings();
            statusBar()->showMessage(tr("Plugins rechargés."), 4000);
        });
        connect(bReset, &QPushButton::clicked, this, [this] {
            if (QMessageBox::question(this, tr("Réinitialiser"),
                                      tr("Réinitialiser les réglages de LuantiCreator (thème, dossiers d'export, dernier projet) ?\n"
                                         "Vos projets et vos plugins ne sont pas touchés.")) != QMessageBox::Yes)
                return;
            QSettings st("LuantiCreator", "LuantiCreator");
            st.clear();
            darkBox_->setChecked(true);
            refreshProjects();
            statusBar()->showMessage(tr("Réglages réinitialisés."), 4000);
        });
        connect(bAbout, &QPushButton::clicked, this, [this] {
            QMessageBox::about(this, "LuantiCreator",
                               tr("LuantiCreator %1\nCréateur visuel de mods pour Luanti.\n\n"
                                  "Dans l'éditeur d'un mod : Ctrl+H : accueil — Ctrl+Shift+H : retour aux projets — Ctrl+E : exporter — "
                                  "F5 : lancer le serveur de test — F6 : rejoindre").arg(LC_VERSION));
        });
        return w;
    }

    // icônes des onglets fournies par les plugins : tab.projects, tab.plugins, tab.settings
    void applyTabIcons()
    {
        static const char *keys[] = {"tab.projects", "tab.plugins", "tab.settings"};
        for (int i = 0; i < 3 && i < tabs_->count(); ++i) tabs_->setTabIcon(i, UiHooks::get(keys[i]));
    }

    void refreshSettings()
    {
        if (!stateLbl_) return;
        int on = 0;
        for (const Plugin &p : plugins().plugins()) on += p.enabled ? 1 : 0;
        stateLbl_->setText(tr("%1 projet(s) — %2 plugin(s) installé(s), %3 activé(s).")
                               .arg(projects_.size()).arg(plugins().plugins().size()).arg(on));
        const QStringList who = plugins().corePlugins();
        coreLbl_->setVisible(!who.isEmpty());
        if (!who.isEmpty())
            coreLbl_->setText(tr("Attention : le comportement de LuantiCreator peut être altéré par un plugin : %1").arg(who.join(", ")));
    }

    QTabWidget *tabs_ = nullptr;
    QLineEdit *search_ = nullptr;
    QListWidget *projList_ = nullptr;
    QTextBrowser *info_ = nullptr;
    QPushButton *bOpen_ = nullptr, *bDup_ = nullptr, *bZip_ = nullptr, *bFolder_ = nullptr, *bDel_ = nullptr;
    PluginPage *pluginPage_ = nullptr;
    QCheckBox *darkBox_ = nullptr;
    QLabel *stateLbl_ = nullptr, *coreLbl_ = nullptr;
    QVector<Proj> projects_;
};

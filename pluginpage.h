#pragma once
// Page « Plugins » (trois usages, voir PluginPageMode) : installation depuis un .zip, activation, création et
// édition de plugins, extraits Lua et scripts du projet, référence du format.

#include <QtWidgets>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <functional>

#include "plugins.h"
#include "texcompose.h"
#include "texturedetect.h"
#include "zipread.h"
#include "uihooks.h"
#include "appcore.h"
#include "codehl.h"
#include "apidocwindow.h"

// Contextes des boutons : la clé « <contexte>.btn.<action> » l'emporte sur « btn.<action> ».
//   home.btn.test                 bouton « Tester mon mod » de l'accueil
//   toolbar.btn.*                 barre d'outils du haut
//   server.btn.*                  page « Serveur de test »
//   editor.btn.*  /  editor.<page>.btn.*     boutons des listes (Ajouter, Dupliquer, Supprimer, …) de toutes les pages / d'une page
//   plugins.btn.*                 gestionnaire de plugins (install new edit export del folder reload)
//   pluginedit.btn.*              fenêtre d'édition d'un plugin (save check newfile image textures modimport block icon)
//   launcher.btn.*                fenêtre des projets (new import open dup export folder del reload reset about)
//   tab.projects / tab.plugins / tab.settings   onglets de la fenêtre des projets
//   page.btn.back  /  page.<page>.btn.back   bouton « Accueil » de toutes les pages / d'une page
//   window.<nom>                  icône d'une fenêtre : launcher editor pluginedit plugins apidoc, et boîtes de dialogue
//                                 picker export newmod newplugin frommod textures addblock addicon (repli : « app »)
//   dialog.<nom>.btn.ok|cancel|accept   boutons des boîtes de dialogue (repli : btn.ok, btn.cancel, btn.accept)
//   menu.<action>                 actions des menus (menu.home menu.save menu.export menu.start …) ; <contexte>.menu.<action>
// Clés d'icônes connues. Avec `plug`, on ajoute celles du plugin édité : les clés de ses pages (fonctionnalités) —
// carte, boutons de la liste, bouton « Accueil » — et ses catégories de blocs (cat.<nom>).
inline QStringList knownIconKeys(const Plugin *plug = nullptr)
{
    QStringList k = {"app"};
    const QStringList pages = {"project", "nodes", "models", "items", "foods", "tools", "crafts", "ores", "biomes", "decos",
                               "entities", "cmds", "pixel", "formspec", "preview", "server", "plugins"};
    for (const char *c : {"project", "nodes", "models", "items", "foods", "tools", "crafts", "ores", "biomes", "decos",
                          "entities", "cmds", "pixel", "formspec", "lua", "server", "folder", "export", "save", "open", "plugins"})
        k << QString("card.") + c;
    const QStringList generic = {"add", "dup", "del", "pick", "start", "restart", "stop", "join", "reset", "openworld",
                                 "zip", "folder", "back", "test", "install", "new", "export", "reload",
                                 "open", "import", "edit", "about", "save", "check", "newfile", "image", "textures",
                                 "modimport", "block", "icon"};
    for (const QString &c : generic) k << "btn." + c;
    k << "home.btn.test";
    for (const char *c : {"start", "restart", "join", "stop"}) k << QString("toolbar.btn.") + c;
    for (const char *c : {"start", "restart", "stop", "join", "reset", "openworld", "zip", "folder"}) k << QString("server.btn.") + c;
    for (const char *c : {"add", "dup", "del", "pick"}) k << QString("editor.btn.") + c;
    for (const QString &pg : {"nodes", "items", "foods", "tools", "crafts", "ores", "biomes", "decos", "entities", "cmds", "formspec"})
        for (const char *c : {"add", "dup", "del"}) k << "editor." + pg + ".btn." + c;
    for (const char *c : {"install", "new", "edit", "export", "del", "folder", "reload"}) k << QString("plugins.btn.") + c;
    for (const char *c : {"save", "check", "newfile", "image", "textures", "modimport", "block", "icon"})
        k << QString("pluginedit.btn.") + c;
    for (const char *c : {"new", "import", "open", "dup", "export", "folder", "del", "reload", "reset", "about"})
        k << QString("launcher.btn.") + c;
    // boutons ajoutés ensuite (voir uihooks.h) : clés générales, puis par contexte
    for (const char *c : {"browse", "detect", "clearlog", "presets", "run", "color", "clear", "apidoc", "prev", "next", "selectbox",
                          "refresh", "addimages", "adddir", "applyall", "addtext", "copy", "ok", "cancel", "accept", "other"})
        k << QString("btn.") + c;
    for (const char *c : {"browse", "detect", "del", "clearlog"}) k << QString("server.btn.") + c;
    k << "editor.btn.presets" << "editor.formspec.btn.addtext" << "editor.formspec.btn.copy";
    k << "plugins.btn.run" << "launcher.btn.apidoc" << "apidoc.btn.prev" << "apidoc.btn.next";
    k << "models.btn.selectbox" << "models.btn.refresh" << "preview.btn.refresh" << "project.btn.folder";
    for (const char *c : {"color", "clear", "open", "save"}) k << QString("pixel.btn.") + c;
    for (const char *w : {"launcher", "editor", "pluginedit", "plugins", "apidoc", "picker", "export", "newmod", "newplugin",
                          "frommod", "textures", "addblock", "addicon"})
        k << QString("window.") + w;
    for (const char *d : {"picker", "export", "newmod", "newplugin", "frommod", "textures", "addblock", "addicon"})
        for (const char *c : {"ok", "cancel", "accept"}) k << QString("dialog.") + d + ".btn." + c;
    k << "dialog.export.btn.browse" << "dialog.textures.btn.addimages" << "dialog.textures.btn.adddir" << "dialog.textures.btn.clear"
      << "dialog.textures.btn.applyall" << "dialog.addblock.btn.pick" << "dialog.addicon.btn.pick";
    for (const char *c : {"home", "projects", "save", "savecopy", "folder", "export", "quit", "dark", "plugins", "start", "restart", "join",
                          "stop", "about", "close", "apidoc", "dup", "front", "back", "del"})
        k << QString("menu.") + c;
    k << "launcher.menu.import.zip" << "launcher.menu.import.folder" << "launcher.menu.import.project";
    for (const char *c : {"save", "close", "apidoc"}) k << QString("pluginedit.menu.") + c;
    for (const char *c : {"dup", "front", "back", "del"}) k << QString("formspec.menu.") + c;
    k << "tab.projects" << "tab.plugins" << "tab.settings";
    k << "page.btn.back";
    for (const QString &pg : pages) k << "page." + pg + ".btn.back";
    for (const char *c : {"mon_mod", "terrain", "pierre", "bois", "minerais", "verre", "objets", "nourriture"})
        k << QString("cat.") + c;
    if (plug) {
        for (const PluginFeature &f : plug->features) { // pages ajoutées par le plugin
            k << "card." + f.key << "page." + f.key + ".btn.back";
            for (const char *c : {"add", "dup", "del"}) k << "editor." + f.key + ".btn." + c;
        }
        for (const PluginBlock &b : plug->blocks)
            if (!b.category.isEmpty()) k << "cat." + identOf(b.category);
        for (auto it = plug->icons.begin(); it != plug->icons.end(); ++it) k << it.key(); // déjà déclarées dans le plugin
    }
    {   // clés détectées dans le code (uikeys.h) et déclarées à l'exécution
        std::size_t n = 0;
        const char *const *ck = lckeys::codeIconKeys(&n);
        for (std::size_t i = 0; i < n; ++i) k << QString::fromUtf8(ck[i]);
        k << UiHooks::seenKeys().values();
    }
    k.removeDuplicates();
    return k;
}

// État d'une clé d'icône dans un plugin :
//   Direct   (vert)   : le plugin déclare une icône pour cette clé ;
//   Enclosed (orange) : pas d'icône pour cette clé, mais une clé englobante en a une (ex : « btn.add » couvre
//                       « editor.nodes.btn.add ») ; `via` reçoit cette clé englobante ;
//   Missing  (rouge)  : aucune icône enregistrée pour cette clé.
enum class IconState { Missing, Enclosed, Direct };

inline IconState iconKeyState(const Plugin &p, const QString &key, QString *via = nullptr)
{
    if (p.icons.contains(key)) return IconState::Direct;
    const int i = key.indexOf(".btn.");
    if (i > 0) { // <contexte>.btn.<action> : clés englobantes = contextes plus larges puis btn.<action>
        const QStringList c = UiHooks::candidates(key.left(i), key.mid(i + 1));
        for (int j = 1; j < c.size(); ++j)
            if (p.icons.contains(c[j])) {
                if (via) *via = c[j];
                return IconState::Enclosed;
            }
    }
    return IconState::Missing;
}

inline QColor iconStateColor(IconState s)
{
    return s == IconState::Direct ? QColor("#43a047") : s == IconState::Enclosed ? QColor("#fb8c00") : QColor("#e53935");
}

inline QString iconStateText(IconState s, const QString &via = QString())
{
    if (s == IconState::Direct) return QObject::tr("Icône enregistrée dans ce plugin");
    if (s == IconState::Enclosed) return QObject::tr("Pas d'icône pour cette clé, mais la clé englobante « %1 » en a une").arg(via);
    return QObject::tr("Icône non enregistrée");
}

inline QIcon iconStateDot(IconState s)
{
    QPixmap px(14, 14);
    px.fill(Qt::transparent);
    QPainter q(&px);
    q.setRenderHint(QPainter::Antialiasing);
    q.setPen(Qt::NoPen);
    q.setBrush(iconStateColor(s));
    q.drawEllipse(1, 1, 12, 12);
    return QIcon(px);
}

// Les trois usages de la page Plugins (chacun dans sa propre fenêtre) :
//   Manage  : fenêtre « Gestionnaire » (lanceur) : installer, créer, activer, autoriser, exporter, supprimer des plugins ;
//   Editor  : fenêtre d'édition d'UN plugin (fichiers, textures, blocs, icônes, référence du format) ;
//   Project : dans l'éditeur d'un mod, extraits Lua et scripts des plugins pour CE mod.
enum class PluginPageMode { Manage, Editor, Project };

class PluginPage : public QWidget {
public:
    std::function<void()> onChanged;          // plugins ajoutés / retirés / activés / modifiés
    std::function<void()> onSnippetsChanged;  // extraits cochés pour le projet
    // exécute un script « manuel » (plugin, script) ; renvoie le texte du journal à afficher
    std::function<QString(const QString &, const QString &)> runScript;
    // Manage : ouvre l'éditeur du plugin (id) dans sa propre fenêtre
    std::function<void(const QString &)> editPlugin;

    PluginPage(PluginPageMode mode = PluginPageMode::Project, QWidget *parent = nullptr) : QWidget(parent), mode_(mode)
    {
        QTabWidget *tabs = nullptr;
        auto *v = new QVBoxLayout(this);
        v->setContentsMargins(0, 0, 0, 0);
        if (mode != PluginPageMode::Manage) {
            tabs = new QTabWidget;
            v->addWidget(tabs);
        }
        auto addPage = [&](QWidget *w, const QString &title) {
            if (tabs) tabs->addTab(w, title);
            else v->addWidget(w);
        };

        // ---------------------------------------------------------- installés (gestionnaire)
        if (mode == PluginPageMode::Manage) {
            auto *inst = new QWidget;
            auto *ih = new QHBoxLayout(inst);
            auto *left = new QVBoxLayout;
            list_ = new QListWidget;
            list_->setIconSize(QSize(24, 24));
            auto *bZip = new QPushButton(tr("Installer depuis un ZIP..."));
            auto *bNew = new QPushButton(tr("Nouveau plugin..."));
            auto *bEdit = new QPushButton(tr("Modifier le plugin (nouvelle fenêtre)..."));
            UiHooks::bind(bEdit, "btn.edit", "plugins");
            bEdit->setToolTip(tr("Ouvre l'éditeur du plugin sélectionné dans une fenêtre séparée (double-clic sur la liste)."));
            auto *bFromMod = new QPushButton(tr("Créer depuis un mod (.zip)..."));
            bFromMod->setToolTip(tr("Crée un plugin à partir du .zip d'un mod Luanti : ses blocs et textures deviennent disponibles dans tous les sélecteurs."));
            auto *bExp = new QPushButton(tr("Exporter en .zip..."));
            auto *bDel = new QPushButton(tr("Supprimer"));
            auto *bOpen = new QPushButton(tr("Ouvrir le dossier"));
            auto *bReload = new QPushButton(tr("Recharger"));
            left->addWidget(list_, 1);
            auto *leftBtns = new QBoxLayout(QBoxLayout::TopToBottom);
            UiHooks::bindRow(leftBtns, {{bZip, "btn.install"}, {bNew, "btn.new"}, {bFromMod, "btn.zip"}, {bExp, "btn.export"},
                                        {bDel, "btn.del"}, {bOpen, "btn.folder"}, {bReload, "btn.reload"}}, "plugins");
            leftBtns->insertWidget(2, bEdit);
            left->addLayout(leftBtns);
            info_ = new LcDocPane(true);
            allowScripts_ = new QCheckBox(tr("Autoriser les scripts de ce plugin"));
            allowScripts_->setVisible(false);
            allowCore_ = new QCheckBox(tr("Autoriser ce plugin à modifier LuantiCreator"));
            allowCore_->setToolTip(tr("Permet au plugin (module Python luanti_creator_core) de modifier le titre, les cartes, les icônes, "
                                      "le style et le code Lua généré. Le comportement de LuantiCreator peut alors être altéré."));
            allowCore_->setVisible(false);
            auto *right = new QVBoxLayout;
            right->addWidget(info_, 1);
            right->addWidget(allowScripts_);
            right->addWidget(allowCore_);
            ih->addLayout(left, 2);
            ih->addLayout(right, 3);
            addPage(inst, tr("Plugins installés"));

            connect(allowScripts_, &QCheckBox::clicked, this, [this](bool on) { toggleScripts(on); });
            connect(allowCore_, &QCheckBox::clicked, this, [this](bool on) { toggleCore(on); });
            connect(list_, &QListWidget::currentRowChanged, this, [this](int) { showInfo(); });
            connect(list_, &QListWidget::itemChanged, this, [this](QListWidgetItem *it) {
                if (building_) return;
                plugins().setEnabled(it->data(Qt::UserRole).toString(), it->checkState() == Qt::Checked);
                showInfo();
                changed();
                reloadWholeApp();
            });
            connect(list_, &QListWidget::itemDoubleClicked, this, [this](QListWidgetItem *it) {
                if (editPlugin && it) editPlugin(it->data(Qt::UserRole).toString());
            });
            connect(bEdit, &QPushButton::clicked, this, [this] {
                if (!editPlugin) return;
                if (currentId().isEmpty()) {
                    QMessageBox::information(this, tr("Plugin"), tr("Sélectionnez d'abord un plugin dans la liste."));
                    return;
                }
                editPlugin(currentId());
            });
            connect(bZip, &QPushButton::clicked, this, [this] { installZip(); });
            connect(bNew, &QPushButton::clicked, this, [this] { newPlugin(); });
            connect(bFromMod, &QPushButton::clicked, this, [this] { createFromModZip(); });
            connect(bExp, &QPushButton::clicked, this, [this] { exportPlugin(); });
            connect(bDel, &QPushButton::clicked, this, [this] { removePlugin(); });
            connect(bOpen, &QPushButton::clicked, this, [this] {
                const QString d = currentDir();
                QDesktopServices::openUrl(QUrl::fromLocalFile(d.isEmpty() ? PluginHost::root() : d));
            });
            connect(bReload, &QPushButton::clicked, this, [this] {
                plugins().reload();
                rebuild();
                changed();
                reloadWholeApp();
            });
        }

        // ---------------------------------------------------------- scripts manuels (mod ouvert)
        if (mode == PluginPageMode::Project) {
            auto *sc = new QWidget;
            auto *scv = new QVBoxLayout(sc);
            auto *scNote = new QLabel(tr("Scripts « manuels » des plugins activés (Python, etc.). Un script peut lire votre projet et y "
                                         "ajouter des éléments. Il s'exécute avec vos droits : seuls les plugins que vous avez autorisés "
                                         "(gestionnaire de plugins, depuis la fenêtre des projets) sont lancés."));
            scNote->setWordWrap(true);
            scripts_ = new QListWidget;
            auto *bRun = new QPushButton(tr("Exécuter le script"));
            UiHooks::bind(bRun, "btn.run", "plugins");
            scriptOut_ = new QPlainTextEdit;
            scriptOut_->setReadOnly(true);
            scriptOut_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
            scv->addWidget(scNote);
            scv->addWidget(scripts_, 1);
            scv->addWidget(bRun);
            scv->addWidget(scriptOut_, 2);
            addPage(sc, tr("Scripts"));
            connect(bRun, &QPushButton::clicked, this, [this] { runSelectedScript(); });
            connect(scripts_, &QListWidget::itemDoubleClicked, this, [this] { runSelectedScript(); });

            // ---------------------------------------------------------- extraits du projet
            auto *sn = new QWidget;
            auto *sv = new QVBoxLayout(sn);
            auto *snNote = new QLabel(tr("Cochez les extraits Lua à ajouter à <b>ce mod</b> (ils sont copiés à la fin de init.lua, "
                                         "régénéré automatiquement). Seuls les plugins activés apparaissent. "
                                         "Les plugins se gèrent depuis la fenêtre des projets (Fichier → Gestionnaire de projets)."));
            snNote->setWordWrap(true);
            snippets_ = new QListWidget;
            snippetInfo_ = new QPlainTextEdit;
            snippetInfo_->setReadOnly(true);
            snippetInfo_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
            lcAttachHighlighter(snippetInfo_, LcLang::Lua);
            sv->addWidget(snNote);
            sv->addWidget(snippets_, 1);
            sv->addWidget(new QLabel(tr("Code de l'extrait sélectionné :")));
            sv->addWidget(snippetInfo_, 2);
            addPage(sn, tr("Extraits Lua du projet"));
            connect(snippets_, &QListWidget::itemChanged, this, [this](QListWidgetItem *it) {
                if (building_) return;
                const QString id = it->data(Qt::UserRole).toString();
                enabledSnippets_.removeAll(id);
                if (it->checkState() == Qt::Checked) enabledSnippets_ << id;
                if (onSnippetsChanged) onSnippetsChanged();
            });
            connect(snippets_, &QListWidget::currentItemChanged, this, [this](QListWidgetItem *it, QListWidgetItem *) {
                const PluginSnippet *s = it ? plugins().snippet(it->data(Qt::UserRole).toString()) : nullptr;
                snippetInfo_->setPlainText(s ? s->text : QString());
            });
        }

        // ---------------------------------------------------------- éditeur (fenêtre d'édition d'un plugin)
        if (mode == PluginPageMode::Editor) {
            auto *ed = new QWidget;
            auto *ev = new QVBoxLayout(ed);
            edTitle_ = new QLabel;
            edTitle_->setStyleSheet("font-weight:bold;");
            files_ = new QComboBox;
            auto *top = new QHBoxLayout;
            top->addWidget(new QLabel(tr("Fichier :")));
            top->addWidget(files_, 1);
            editor_ = new QPlainTextEdit;
            editor_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
            editor_->setLineWrapMode(QPlainTextEdit::NoWrap);
            editorHl_ = lcAttachHighlighter(editor_, LcLang::None); // langage choisi d'après le fichier ouvert (openFile)
            status_ = new QLabel;
            status_->setWordWrap(true);
            auto *bSave = new QPushButton(tr("Enregistrer"));
            bSave->setShortcut(QKeySequence("Ctrl+S"));
            auto *bCheck = new QPushButton(tr("Valider le plugin"));
            auto *bNewFile = new QPushButton(tr("Nouveau fichier..."));
            auto *bImport = new QPushButton(tr("Importer une image..."));
            auto *bTex = new QPushButton(tr("Importer des textures..."));
            bTex->setToolTip(tr("Importe plusieurs textures d'un coup et détecte automatiquement le mod et le bloc (default_dirt.png -> default:dirt)."));
            auto *bModImport = new QPushButton(tr("Importer depuis un mod..."));
            bModImport->setToolTip(tr("Ajoute au plugin ouvert les blocs et textures d'un mod (.zip) : noms des blocs lus dans son code, textures récupérées."));
            auto *bBlock = new QPushButton(tr("Bloc..."));
            auto *bIcon = new QPushButton(tr("Icône..."));
            auto *btns = new QHBoxLayout;
            for (QPushButton *b : {bSave, bCheck, bNewFile, bImport, bTex, bModImport, bBlock, bIcon}) btns->addWidget(b);
            UiHooks::bind(bSave, "btn.save", "pluginedit");
            UiHooks::bind(bCheck, "btn.check", "pluginedit");
            UiHooks::bind(bNewFile, "btn.newfile", "pluginedit");
            UiHooks::bind(bImport, "btn.image", "pluginedit");
            UiHooks::bind(bTex, "btn.textures", "pluginedit");
            UiHooks::bind(bModImport, "btn.modimport", "pluginedit");
            UiHooks::bind(bBlock, "btn.block", "pluginedit");
            UiHooks::bind(bIcon, "btn.icon", "pluginedit");
            ev->addWidget(edTitle_);
            ev->addLayout(top);
            ev->addWidget(editor_, 1);
            ev->addLayout(btns);
            ev->addWidget(status_);
            addPage(ed, tr("Coder un plugin"));

            auto *ref = new LcDocPane;
            ref->setMarkdown(referenceMarkdown());
            addPage(ref, tr("Référence du format"));

            connect(files_, &QComboBox::currentTextChanged, this, [this](const QString &f) { openFile(f); });
            connect(bSave, &QPushButton::clicked, this, [this] { saveFile(); });
            connect(bCheck, &QPushButton::clicked, this, [this] { validate(); });
            connect(bNewFile, &QPushButton::clicked, this, [this] { newFile(); });
            connect(bImport, &QPushButton::clicked, this, [this] { importImage(); });
            connect(bTex, &QPushButton::clicked, this, [this] { importTextures(); });
            connect(bModImport, &QPushButton::clicked, this, [this] { createFromModZip(true); });
            connect(bBlock, &QPushButton::clicked, this, [this] { addBlock(); });
            connect(bIcon, &QPushButton::clicked, this, [this] { addIcon(); });
        }

        rebuild();
    }

    QStringList enabledSnippets() const { return enabledSnippets_; }
    void setEnabledSnippets(const QStringList &l)
    {
        enabledSnippets_ = l;
        if (snippets_) rebuildSnippets();
    }

    // Editor : désigne le plugin édité par cette fenêtre
    void setPlugin(const QString &id)
    {
        curId_ = id;
        rebuild();
    }
    QString pluginId() const { return currentId(); }

    // Editor : modifications non enregistrées dans le fichier ouvert ?
    bool isDirty() const { return editor_ && editor_->document()->isModified(); }
    // enregistre le fichier ouvert ; false si l'enregistrement a échoué (erreur de syntaxe du manifeste, etc.)
    bool saveCurrent()
    {
        if (!isDirty()) return true;
        saveFile();
        return !isDirty();
    }

    // recharge l'affichage après un changement de plugins
    void rebuild()
    {
        building_ = true;
        const QString keep = currentId();
        if (list_) {
            list_->clear();
            for (const Plugin &p : plugins().plugins()) {
                auto *it = new QListWidgetItem(p.name + (p.version.isEmpty() ? QString() : "  v" + p.version));
                it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
                it->setCheckState(p.enabled ? Qt::Checked : Qt::Unchecked);
                it->setData(Qt::UserRole, p.id);
                if (!p.problems.isEmpty()) it->setForeground(QColor("#e08a00"));
                list_->addItem(it);
            }
            selectId(keep);
            if (list_->currentRow() < 0 && list_->count()) list_->setCurrentRow(0);
        }
        building_ = false;
        if (snippets_) rebuildSnippets();
        if (scripts_) rebuildScripts();
        if (info_) showInfo();
        if (files_) loadFileList();
    }

private:
    // recharge toute l'application (plugins, style, fenêtres) : demandé à chaque validation / enregistrement d'un plugin
    void reloadWholeApp()
    {
        if (mode_ != PluginPageMode::Project && AppFlow::reloadApp) AppFlow::reloadApp();
    }

    void changed()
    {
        if (onChanged) onChanged();
        if (mode_ != PluginPageMode::Project) AppBus::notify(this, "plugins"); // les autres fenêtres se mettent à jour
    }

    QString currentId() const
    {
        if (!list_) return curId_; // fenêtre d'édition : un seul plugin, désigné par setPlugin()
        auto *it = list_->currentItem();
        return it ? it->data(Qt::UserRole).toString() : QString();
    }
    void selectId(const QString &id)
    {
        if (!list_) {
            curId_ = id;
            return;
        }
        for (int i = 0; i < list_->count(); ++i)
            if (list_->item(i)->data(Qt::UserRole).toString() == id) list_->setCurrentRow(i);
    }
    const Plugin *current() const { return plugins().find(currentId()); }
    QString currentDir() const
    {
        const Plugin *p = current();
        return p ? p->dir : QString();
    }

    void rebuildSnippets()
    {
        building_ = true;
        snippets_->clear();
        for (const Plugin &p : plugins().plugins()) {
            if (!p.enabled) continue;
            for (const PluginSnippet &s : p.snippets) {
                const QString full = p.id + "/" + s.id;
                auto *it = new QListWidgetItem(QString("%1 — %2").arg(p.name, s.name) +
                                               (s.desc.isEmpty() ? QString() : "  (" + s.desc + ")"));
                it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
                it->setCheckState(enabledSnippets_.contains(full) ? Qt::Checked : Qt::Unchecked);
                it->setData(Qt::UserRole, full);
                snippets_->addItem(it);
            }
        }
        building_ = false;
        snippetInfo_->clear();
    }

    void showInfo()
    {
        if (!info_) return; // seul le gestionnaire affiche la fiche du plugin
        const Plugin *p = current();
        allowScripts_->setVisible(p && p->hasScripts());
        if (p && p->hasScripts()) allowScripts_->setChecked(plugins().scriptsAllowed(*p));
        allowCore_->setVisible(p && p->core);
        if (p && p->core) {
            allowCore_->setChecked(plugins().coreAllowed(*p));
            allowCore_->setEnabled(plugins().scriptsAllowed(*p)); // sans scripts autorisés, rien ne s'exécute
        }
        auto escape = [](QString text) {
            static const QString special = "\\`*_{}[]()#+-.!|~<>";
            QString out;
            out.reserve(text.size() * 2);
            for (QChar c : text) {
                if (special.contains(c)) out += '\\';
                out += c;
            }
            return out;
        };
        auto code = [](QString text) {
            text.replace('`', "\\`");
            return "`" + text + "`";
        };
        if (!p) {
            info_->setMarkdown(tr("## Aucun plugin installé\n\nInstallez un plugin depuis un fichier **.zip** ou créez le vôtre avec "
                                  "« Nouveau plugin ».\n\nDossier des plugins :\n\n%1")
                                   .arg(code(PluginHost::root())));
            return;
        }
        QString markdown = "## " + escape(p->name) + "\n\n" + escape(p->description) + "\n\n";
        if (p->bundled)
            markdown += tr("> **Plugin intégré à l'exécutable.** Supprimé, il ne revient qu'avec une nouvelle version de l'archive.\n\n");
        markdown += tr("Identifiant : %1 — version %2 — auteur : %3\n\n")
                        .arg(code(p->id), escape(p->version), escape(p->author.isEmpty() ? "?" : p->author));
        markdown += tr("- %1 icône(s)\n- %2 bloc(s) pour les sélecteurs\n- %3 texture(s) de remplacement\n"
                       "- %4 modèle(s) prédéfini(s)\n- %5 extrait(s) Lua\n- %6 fonctionnalité(s)\n")
                        .arg(p->icons.size()).arg(p->blocks.size()).arg(p->textures.size()).arg(p->presets.size())
                        .arg(p->snippets.size()).arg(p->features.size());
        if (p->hasScripts()) {
            int n = p->scripts.size();
            for (const PluginFeature &f : p->features)
                if (!f.script.isEmpty()) ++n;
            markdown += plugins().scriptsAllowed(*p)
                            ? tr("\n**%1 script(s)** — autorisés.\n").arg(n)
                            : tr("\n> **%1 script(s) — non autorisés** : ils ne s'exécutent pas tant que vous n'avez pas coché "
                                 "« Autoriser les scripts de ce plugin ».\n").arg(n);
        }
        if (p->core) {
            markdown += plugins().coreAllowed(*p) && plugins().scriptsAllowed(*p)
                            ? tr("\n> **Attention : ce plugin peut modifier LuantiCreator** (module %1) : son comportement peut être altéré. "
                                 "Un avertissement reste affiché en bas de la fenêtre.\n").arg(code("luanti_creator_core"))
                            : tr("\n> Ce plugin demande à **modifier LuantiCreator** (titre, cartes, icônes, style, code Lua généré). "
                                 "Rien n'est modifié tant que vous n'avez pas autorisé les scripts puis coché « Autoriser ce plugin à modifier LuantiCreator ».\n");
        }
        if (!p->features.isEmpty()) {
            markdown += tr("\n### Clés d'icône des pages de ce plugin\n\nÀ utiliser dans `icons` :\n\n");
            for (const PluginFeature &f : p->features) {
                const QStringList keys = {"card." + f.key, "editor." + f.key + ".btn.add", "editor." + f.key + ".btn.dup",
                                          "editor." + f.key + ".btn.del", "page." + f.key + ".btn.back"};
                markdown += "**" + escape(f.title) + "**\n\n";
                for (const QString &k : keys) {
                    QString via;
                    const IconState state = iconKeyState(*p, k, &via);
                    const QString status = state == IconState::Direct ? tr("enregistrée")
                                            : state == IconState::Enclosed ? tr("couverte")
                                                                           : tr("non enregistrée");
                    markdown += QString("- %1 — %2\n").arg(escape(status), code(k));
                }
                markdown += "\n";
            }
            markdown += tr("Non enregistrée : aucune icône ; couverte : une clé englobante fournit une icône ; enregistrée : icône définie.\n");
        }
        if (!p->enabled) markdown += tr("\n*Plugin désactivé.*\n");
        if (!p->problems.isEmpty()) {
            markdown += tr("\n### Problèmes\n\n");
            for (const QString &problem : p->problems) markdown += "- " + escape(problem) + "\n";
        }
        info_->setMarkdown(markdown);
    }

    // ------------------------------------------------------------ scripts
    void rebuildScripts()
    {
        scripts_->clear();
        for (const Plugin &p : plugins().plugins()) {
            if (!p.enabled) continue;
            for (const PluginScript &s : p.scripts) {
                if (s.event != "manual") continue;
                auto *it = new QListWidgetItem(QString("%1 — %2").arg(p.name, s.name));
                it->setToolTip(s.desc);
                it->setData(Qt::UserRole, p.id);
                it->setData(Qt::UserRole + 1, s.id);
                scripts_->addItem(it);
            }
        }
        if (scripts_->count()) scripts_->setCurrentRow(0);
    }

    // coche / décoche « Autoriser les scripts » (avec confirmation explicite)
    void toggleScripts(bool on)
    {
        const Plugin *p = current();
        if (!p) return;
        if (on) {
            const QString m = tr("Autoriser les scripts du plugin « %1 » ?\n\nUn script est un programme (Python, shell...) qui "
                                 "s'exécute avec VOS droits : il peut lire, modifier ou supprimer vos fichiers et utiliser le réseau. "
                                 "LuantiCreator ne l'isole pas.\n\nN'autorisez que les plugins dont vous avez lu les scripts ou "
                                 "auxquels vous faites confiance. L'autorisation est retirée si le plugin est modifié ou remplacé.")
                                  .arg(p->name);
            if (QMessageBox::warning(this, tr("Autoriser les scripts"), m, QMessageBox::Yes | QMessageBox::Cancel,
                                     QMessageBox::Cancel) != QMessageBox::Yes) {
                allowScripts_->setChecked(false);
                return;
            }
        }
        plugins().setScriptsAllowed(*p, on);
        showInfo();
        changed(); // scripts « startup » / avertissement de modification de LuantiCreator à jour
        reloadWholeApp();
    }

    // coche / décoche « Autoriser ce plugin à modifier LuantiCreator » (avec avertissement explicite)
    void toggleCore(bool on)
    {
        const Plugin *p = current();
        if (!p) return;
        if (on) {
            const QString m = tr("Autoriser le plugin « %1 » à modifier LuantiCreator ?\n\n"
                                 "Attention : le comportement de LuantiCreator peut être altéré à cause de ce plugin : il pourra changer le titre de la "
                                 "fenêtre, renommer ou masquer des cartes, remplacer des icônes, modifier le style de l'application et "
                                 "transformer le code Lua généré de vos mods.\n\n"
                                 "Un avertissement permanent s'affichera en bas de la fenêtre tant que ce plugin est actif. "
                                 "Si quelque chose se comporte de façon inattendue, désactivez le plugin.\n\n"
                                 "N'autorisez que des plugins de confiance. L'autorisation est retirée si le plugin est modifié ou remplacé.")
                                  .arg(p->name);
            if (QMessageBox::warning(this, tr("Modifier LuantiCreator"), m, QMessageBox::Yes | QMessageBox::Cancel,
                                     QMessageBox::Cancel) != QMessageBox::Yes) {
                allowCore_->setChecked(false);
                return;
            }
        }
        plugins().setCoreAllowed(*p, on);
        showInfo();
        changed();
        reloadWholeApp(); // le style / les modifications du plugin s'appliquent (ou disparaissent) tout de suite
    }

    void runSelectedScript()
    {
        auto *it = scripts_->currentItem();
        if (!it || !runScript) return;
        const QString pid = it->data(Qt::UserRole).toString(), sid = it->data(Qt::UserRole + 1).toString();
        const Plugin *p = plugins().find(pid);
        if (!p) return;
        if (!plugins().scriptsAllowed(*p)) {
            const QString m = tr("Les scripts du plugin « %1 » ne sont pas autorisés.\n\nUn script s'exécute avec VOS droits "
                                 "(fichiers, réseau...). L'autoriser maintenant et l'exécuter ?").arg(p->name);
            if (QMessageBox::warning(this, tr("Autoriser les scripts"), m, QMessageBox::Yes | QMessageBox::Cancel,
                                     QMessageBox::Cancel) != QMessageBox::Yes)
                return;
            plugins().setScriptsAllowed(*p, true);
            showInfo();
        }
        scriptOut_->setPlainText(tr("Exécution…"));
        QApplication::setOverrideCursor(Qt::WaitCursor);
        const QString out = runScript(pid, sid);
        QApplication::restoreOverrideCursor();
        scriptOut_->setPlainText(out);
        changed(); // les pages ont pu recevoir de nouveaux éléments
    }

    // l'utilisateur modifie lui-même son plugin : une autorisation existante est conservée
    void rememberTrust()
    {
        const Plugin *p = current();
        trust_ = p && plugins().scriptsAllowed(*p);
        trustCore_ = p && plugins().coreAllowed(*p);
    }

    // ------------------------------------------------------------ installation
    void installZip()
    {
        const QString zip = QFileDialog::getOpenFileName(this, tr("Plugin LuantiCreator (.zip)"), QDir::homePath(),
                                                         tr("Archive zip (*.zip);;Tous les fichiers (*)"));
        if (zip.isEmpty()) return;
        ZipPluginInfo info;
        QString err;
        if (!plugins().inspectZip(zip, info, err)) {
            QMessageBox::warning(this, tr("Plugin"), err);
            return;
        }
        QString msg = tr("Installer le plugin « %1 » (v%2) ?\n\nAuteur : %3\n%4\n\nContenu : %5 icône(s), %6 bloc(s), "
                         "%7 modèle(s), %8 extrait(s) Lua, %9 fonctionnalité(s).")
                          .arg(info.name, info.version, info.author.isEmpty() ? "?" : info.author, info.description)
                          .arg(info.icons).arg(info.blocks).arg(info.presets).arg(info.snippets).arg(info.features);
        if (info.scripts)
            msg += tr("\n\nAttention : ce plugin contient %1 SCRIPT(S) (Python, etc.). Un script est un programme qui s'exécute avec VOS droits "
                      "d'utilisateur (fichiers, réseau...). Il ne tournera que si vous cochez ensuite « Autoriser les scripts de ce "
                      "plugin ». N'installez que des plugins de confiance et lisez leurs scripts avant de les autoriser.").arg(info.scripts);
        else if (info.snippets || info.features)
            msg += tr("\n\nCe plugin peut ajouter du code Lua à vos mods. Il ne s'exécute pas dans LuantiCreator, mais "
                      "n'installez que des plugins de confiance.");
        if (info.skipped) msg += tr("\n\n%1 fichier(s) de type non autorisé seront ignorés.").arg(info.skipped);
        if (info.exists) msg += tr("\n\nUn plugin « %1 » est déjà installé : il sera remplacé.").arg(info.id);
        if (QMessageBox::question(this, tr("Installer un plugin"), msg) != QMessageBox::Yes) return;
        QString id;
        if (!plugins().installZip(zip, id, err)) {
            QMessageBox::warning(this, tr("Plugin"), err);
            return;
        }
        rebuild();
        selectId(id);
        changed();
        const Plugin *p = plugins().find(id);
        if (p && !p->problems.isEmpty())
            QMessageBox::warning(this, tr("Plugin installé avec des problèmes"), p->problems.join("\n"));
    }

    void newPlugin()
    {
        LcDialog dlg(this);
        dlg.setWindowTitle(tr("Nouveau plugin"));
        UiHooks::bindWindow(&dlg, "newplugin");
        auto *form = new QFormLayout(&dlg);
        auto *idEdit = new QLineEdit("mon_plugin");
        auto *nameEdit = new QLineEdit;
        auto *fmtBox = new QComboBox;
        fmtBox->addItems({tr("YAML (plugin.yaml)"), tr("XML (plugin.xml)")});
        auto *emptyBox = new QCheckBox(tr("Plugin vide : je pars de zéro (aucun exemple)"));
        emptyBox->setToolTip(tr("Décoché : le plugin contient un exemple (modèle de biome, extrait Lua, fonctionnalité).\n"
                                "Coché : seulement le manifeste, à remplir vous-même."));
        form->addRow(tr("Identifiant (a-z, 0-9, _)"), idEdit);
        form->addRow(tr("Nom affiché"), nameEdit);
        form->addRow(tr("Format du manifeste"), fmtBox);
        form->addRow(emptyBox);
        auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        UiHooks::bindBox(bb, "newplugin");
        form->addRow(bb);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        if (dlg.exec() != QDialog::Accepted || idEdit->text().trimmed().isEmpty()) return;
        const QString id = idEdit->text().trimmed();
        const QString name = nameEdit->text().trimmed().isEmpty() ? id : nameEdit->text().trimmed();
        QString newId, err;
        if (!plugins().createSkeleton(id, name, newId, err, fmtBox->currentIndex() == 1 ? "xml" : "yaml", emptyBox->isChecked())) {
            QMessageBox::warning(this, tr("Plugin"), err);
            return;
        }
        rebuild();
        selectId(newId);
        changed();
    }

    // Crée un plugin à partir du .zip d'un mod : le mod est lu par LuantiCreator (rien n'est exécuté), ses textures
    // et ses blocs (register_node + tiles dans les .lua, sinon d'après les noms de fichiers) deviennent des blocs du plugin.
    // intoCurrent = false : crée un nouveau plugin ; true : ajoute les blocs au plugin ouvert dans l'éditeur
    void createFromModZip(bool intoCurrent = false)
    {
        if (intoCurrent && (!current() || currentDir().isEmpty())) {
            QMessageBox::information(this, tr("Plugin"), tr("Sélectionnez d'abord un plugin (onglet « Plugins installés »)."));
            return;
        }
        const QString zipPath = QFileDialog::getOpenFileName(this, tr("Mod Luanti (.zip)"), QDir::homePath(),
                                                             tr("Archive zip (*.zip);;Tous les fichiers (*)"));
        if (zipPath.isEmpty()) return;
        QFile zf(zipPath);
        if (zf.size() > 400LL * 1024 * 1024 || !zf.open(QIODevice::ReadOnly)) {
            QMessageBox::warning(this, tr("Plugin"), tr("Archive illisible ou trop volumineuse : %1").arg(zipPath));
            return;
        }
        const QByteArray raw = zf.readAll();
        zf.close();
        const std::string z(raw.constData(), (size_t)raw.size());
        std::vector<lczip::Entry> all, files;
        std::string err;
        if (!lczip::listEntries(z, all, err) || !lczip::filterFiles(all, files, err)) {
            QMessageBox::warning(this, tr("Plugin"), QString::fromStdString(err));
            return;
        }
        const std::vector<std::string> roots = lczip::findModRoots(files);
        if (roots.empty()) {
            QMessageBox::warning(this, tr("Plugin"), tr("Aucun mod trouvé dans cette archive (ni mod.conf ni init.lua)."));
            return;
        }
        std::string root = roots[0];
        if (roots.size() > 1) { // modpack : on choisit le mod à convertir
            QStringList items;
            for (const std::string &r : roots) items << (r.empty() ? tr("(racine)") : QString::fromStdString(r));
            bool ok = false;
            const QString pick = QInputDialog::getItem(this, tr("Modpack"), tr("Cette archive contient plusieurs mods. Lequel convertir ?"), items, 0, false, &ok);
            if (!ok) return;
            root = roots[(size_t)items.indexOf(pick)];
        }
        auto find = [&](const std::string &name) -> const lczip::Entry * {
            for (const lczip::Entry &e : files)
                if (e.name == name) return &e;
            return nullptr;
        };
        auto readText = [&](const std::string &name) {
            std::string out, e2;
            if (const lczip::Entry *e = find(name)) lczip::readEntry(z, *e, out, e2);
            return out;
        };
        const lczip::ModMeta meta = lczip::parseMeta(readText(root + "mod.conf"), readText(root + "depends.txt"));
        std::string modName = meta.name;
        if (modName.empty()) {
            std::string r = root;
            if (!r.empty() && r.back() == '/') r.pop_back();
            const size_t sl = r.rfind('/');
            modName = sl == std::string::npos ? r : r.substr(sl + 1);
            if (modName.empty()) modName = QFileInfo(zipPath).completeBaseName().toStdString();
        }
        modName = lczip::sanitizeName(modName);

        // textures : <mod>/textures/*.png (un seul niveau, comme Luanti)
        const std::string texDir = root + "textures/";
        QMap<QString, const lczip::Entry *> textures; // nom de fichier -> entrée
        for (const lczip::Entry &e : files) {
            if (e.name.compare(0, texDir.size(), texDir) != 0) continue;
            const std::string rest = e.name.substr(texDir.size());
            if (rest.empty() || rest.find('/') != std::string::npos) continue;
            const QString ext = QFileInfo(QString::fromStdString(rest)).suffix().toLower();
            if (ext == "png" || ext == "jpg" || ext == "jpeg" || ext == "bmp") textures[QString::fromStdString(rest)] = &e;
        }

        // image d'un fichier de l'archive
        auto loadTex = [&](const QString &file) -> QImage {
            const auto it = textures.constFind(file);
            if (it == textures.constEnd()) return QImage();
            std::string data, e2;
            if (!lczip::readEntry(z, **it, data, e2)) return QImage();
            return QImage::fromData(QByteArray(data.data(), (int)data.size())).convertToFormat(QImage::Format_ARGB32);
        };
        // Une « texture » de bloc est une expression Luanti « base.png^par_dessus.png » : la base, avec les textures
        // posées par-dessus fusionnées (ex. le côté de default:dirt_with_grass = default_dirt.png + default_grass_side.png).
        auto specImage = [&](const QString &spec) -> QImage {
            const QStringList parts = spec.split('^', Qt::SkipEmptyParts);
            if (parts.isEmpty()) return QImage();
            const QImage base = loadTex(parts[0]);
            if (base.isNull()) return QImage();
            QList<QImage> ovs;
            for (int i = 1; i < parts.size(); ++i) {
                const QImage o = loadTex(parts[i]);
                if (!o.isNull()) ovs << o;
            }
            if (ovs.isEmpty()) return base;
            return lctx::compose(base, ovs, 100, lctx::animFor(QVariantMap(), base));
        };
        auto specUsable = [&](const std::string &spec) {
            const QStringList p = QString::fromStdString(spec).split('^', Qt::SkipEmptyParts);
            return !p.isEmpty() && textures.contains(p[0]);
        };
        auto specLabel = [](const QString &spec) { return QString(spec).replace('^', " + "); };

        struct Row { QString ref, file, source; bool checked; QStringList cands; }; // file : texture (expression) ; cands : possibles pour ce bloc
        QVector<Row> rows;
        QSet<QString> refs, usedFiles;
        int nodesFound = 0;
        // 1) vrais blocs déclarés dans les fichiers Lua
        for (const lczip::Entry &e : files) {
            if (e.name.compare(0, root.size(), root) != 0 || e.name.size() < 5 || e.name.compare(e.name.size() - 4, 4, ".lua") != 0) continue;
            if (e.usize > 2u * 1024u * 1024u) continue;
            std::string lua, e2;
            if (!lczip::readEntry(z, e, lua, e2)) continue;
            for (const lctex::LuaNode &n : lctex::scanNodes(lua)) {
                const QString ref = QString::fromStdString(n.ref);
                if (refs.contains(ref)) continue;
                QStringList ex; // textures de ce bloc dont la base existe dans l'archive, dans l'ordre de « tiles »
                for (const std::string &t : n.tiles) {
                    const QString qt = QString::fromStdString(t);
                    if (specUsable(t) && !ex.contains(qt)) ex << qt;
                }
                if (ex.isEmpty()) continue;
                // texture de face (voir lctex::faceTileIndex), avec ses textures par-dessus ; sinon la première
                const QString tex = specUsable(n.texture) ? QString::fromStdString(n.texture) : ex.first();
                refs.insert(ref);
                for (const QString &x : ex)
                    for (const QString &part : x.split('^', Qt::SkipEmptyParts)) usedFiles.insert(part);
                rows.push_back(Row{ref, tex, tr("nœud Lua"), true, ex});
                ++nodesFound;
            }
        }
        // 2) textures restantes : référence devinée d'après le nom du fichier (le mod est déjà connu)
        {
            const QStringList names = textures.keys();
            std::vector<std::string> sn;
            for (const QString &n : names) sn.push_back(n.toStdString());
            const std::vector<lctex::Detected> det = lctex::detect(sn);
            // plusieurs fichiers pour un même bloc (default_grass.png, default_grass_side.png...) : la face est préférée
            QStringList order;
            QMap<QString, QStringList> groups;
            QMap<QString, int> bestRank;
            QMap<QString, QString> bestFile;
            for (size_t i = 0; i < det.size(); ++i) {
                if (usedFiles.contains(names[(int)i])) continue;
                const std::string full = det[i].mod.empty() ? det[i].name : det[i].mod + "_" + det[i].name;
                const std::string pre = modName + "_";
                const std::string nm = full.compare(0, pre.size(), pre) == 0 && full.size() > pre.size() ? full.substr(pre.size()) : full;
                const QString ref = QString::fromStdString(modName + ":" + nm);
                if (!groups.contains(ref)) order << ref;
                groups[ref] << names[(int)i];
                const int rank = lctex::variantRank(det[i].variant);
                if (!bestRank.contains(ref) || rank < bestRank[ref]) {
                    bestRank[ref] = rank;
                    bestFile[ref] = names[(int)i];
                }
            }
            for (const QString &ref : order) {
                if (refs.contains(ref)) continue;
                refs.insert(ref);
                rows.push_back(Row{ref, bestFile[ref], tr("nom du fichier"), nodesFound == 0, groups[ref]});
            }
        }
        if (rows.isEmpty()) {
            QMessageBox::warning(this, tr("Plugin"), tr("Aucune texture exploitable dans le dossier textures/ de ce mod."));
            return;
        }

        LcDialog dlg(this);
        dlg.setWindowTitle(intoCurrent ? tr("Importer depuis un mod dans « %1 »").arg(current()->name) : tr("Créer un plugin depuis un mod"));
        UiHooks::bindWindow(&dlg, "frommod");
        dlg.resize(760, 520);
        auto *v = new QVBoxLayout(&dlg);
        auto *info = new QLabel(tr("Mod détecté : <b>%1</b> — %2 bloc(s) trouvé(s) dans le code Lua, %3 texture(s) au total.<br>"
                                   "Les blocs issus du code Lua sont cochés ; les textures restantes (objets, particules…) ne le sont que "
                                   "si aucun bloc n'a été trouvé. Pour un bloc à plusieurs textures, la texture de face est choisie ; "
                                   "<b>double-clic sur la case de la texture</b> pour en choisir une autre. Une texture « A + B » est la texture A avec B posée "
                                   "par-dessus (ex. terre + côté d'herbe) : elles sont fusionnées en une image. La référence est modifiable.")
                                    .arg(QString::fromStdString(modName).toHtmlEscaped()).arg(nodesFound).arg(textures.size()));
        info->setWordWrap(true);
        info->setTextFormat(Qt::RichText);
        v->addWidget(info);
        auto *formBox = new QWidget; // identifiant / nom / format : seulement pour un nouveau plugin
        auto *form = new QFormLayout(formBox);
        form->setContentsMargins(0, 0, 0, 0);
        auto *idEdit = new QLineEdit(QString::fromStdString(modName) + "_blocs");
        auto *nameEdit = new QLineEdit(QString::fromStdString(meta.title.empty() ? modName : meta.title));
        auto *fmtBox = new QComboBox;
        fmtBox->addItems({tr("YAML (plugin.yaml)"), tr("XML (plugin.xml)")});
        form->addRow(tr("Identifiant du plugin (a-z, 0-9, _)"), idEdit);
        form->addRow(tr("Nom affiché"), nameEdit);
        form->addRow(tr("Format du manifeste"), fmtBox);
        v->addWidget(formBox);
        formBox->setVisible(!intoCurrent);
        auto *table = new QTableWidget(rows.size(), 3);
        table->setHorizontalHeaderLabels({tr("Référence"), tr("Texture"), tr("Origine")});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setIconSize(QSize(24, 24));
        table->horizontalHeader()->setStretchLastSection(true);
        table->setColumnWidth(0, 280);
        table->setColumnWidth(1, 260);
        for (int r = 0; r < rows.size(); ++r) {
            const QPixmap pm = QPixmap::fromImage(specImage(rows[r].file));
            auto *ri = new QTableWidgetItem(rows[r].ref);
            ri->setFlags(ri->flags() | Qt::ItemIsUserCheckable);
            ri->setCheckState(rows[r].checked ? Qt::Checked : Qt::Unchecked);
            table->setItem(r, 0, ri);
            auto *ti = new QTableWidgetItem(QIcon(pm.scaled(24, 24, Qt::KeepAspectRatio, Qt::FastTransformation)), specLabel(rows[r].file));
            ti->setToolTip(rows[r].file.contains('^') ? tr("Texture de base + texture(s) posée(s) par-dessus, fusionnées dans le plugin") : rows[r].file);
            ti->setFlags(ti->flags() & ~Qt::ItemIsEditable);
            table->setItem(r, 1, ti);
            auto *oi = new QTableWidgetItem(rows[r].source);
            oi->setFlags(oi->flags() & ~Qt::ItemIsEditable);
            table->setItem(r, 2, oi);
        }
        v->addWidget(table, 1);

        // panneau de choix de la texture (double-clic sur la case de la texture)
        auto *panel = new QWidget;
        auto *pv = new QVBoxLayout(panel);
        pv->setContentsMargins(0, 0, 0, 0);
        auto *panelHead = new QHBoxLayout;
        auto *panelLbl = new QLabel;
        panelLbl->setWordWrap(true);
        auto *allBox = new QCheckBox(tr("Toutes les textures du mod"));
        panelHead->addWidget(panelLbl, 1);
        panelHead->addWidget(allBox);
        auto *texList = new QListWidget;
        texList->setViewMode(QListView::IconMode);
        texList->setIconSize(QSize(48, 48));
        texList->setResizeMode(QListView::Adjust);
        texList->setMovement(QListView::Static);
        texList->setWrapping(true);
        texList->setFixedHeight(116);
        pv->addLayout(panelHead);
        pv->addWidget(texList);
        panel->hide();
        v->addWidget(panel);
        int panelRow = -1;
        QHash<QString, QIcon> iconCache;
        auto iconOf = [&](const QString &spec) {
            if (!iconCache.contains(spec))
                iconCache.insert(spec, QIcon(QPixmap::fromImage(specImage(spec)).scaled(48, 48, Qt::KeepAspectRatio, Qt::FastTransformation)));
            return iconCache.value(spec);
        };
        auto fillTexList = [&] {
            texList->clear();
            if (panelRow < 0 || panelRow >= rows.size()) return;
            const QStringList list = (allBox->isChecked() || rows[panelRow].cands.isEmpty()) ? textures.keys() : rows[panelRow].cands;
            for (const QString &f : list) {
                const QString lab = specLabel(f);
                auto *it = new QListWidgetItem(iconOf(f), lab.length() > 18 ? lab.left(16) + "…" : lab);
                it->setToolTip(lab);
                it->setData(Qt::UserRole, f);
                texList->addItem(it);
                if (f == rows[panelRow].file) texList->setCurrentItem(it);
            }
        };
        connect(table, &QTableWidget::cellDoubleClicked, &dlg, [&](int r, int c) {
            if (c != 1 || r < 0 || r >= rows.size()) return;
            panelRow = r;
            panelLbl->setText(tr("Texture de <b>%1</b> — cliquez sur celle à utiliser :").arg(table->item(r, 0)->text().toHtmlEscaped()));
            fillTexList();
            panel->show();
        });
        connect(allBox, &QCheckBox::toggled, &dlg, [&](bool) { fillTexList(); });
        connect(texList, &QListWidget::itemClicked, &dlg, [&](QListWidgetItem *it) {
            if (panelRow < 0 || panelRow >= rows.size()) return;
            const QString file = it->data(Qt::UserRole).toString();
            rows[panelRow].file = file;
            table->item(panelRow, 1)->setText(specLabel(file));
            table->item(panelRow, 1)->setIcon(iconOf(file));
        });

        auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        UiHooks::bindBox(bb, "frommod");
        v->addWidget(bb);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        if (dlg.exec() != QDialog::Accepted || (!intoCurrent && idEdit->text().trimmed().isEmpty())) return;

        struct Pick { QString ref, file; };
        QVector<Pick> picks;
        QSet<QString> seen;
        for (int r = 0; r < table->rowCount(); ++r) {
            if (table->item(r, 0)->checkState() != Qt::Checked) continue;
            const QString ref = table->item(r, 0)->text().trimmed().toLower();
            if (ref.count(':') != 1 || ref.startsWith(':') || ref.endsWith(':') || seen.contains(ref)) continue;
            seen.insert(ref);
            picks.push_back(Pick{ref, rows[r].file});
        }
        if (picks.isEmpty()) {
            QMessageBox::information(this, tr("Plugin"), tr("Aucun bloc coché (ou références invalides) : rien n'a été créé."));
            return;
        }
        if (!intoCurrent) {
            const QString name = nameEdit->text().trimmed().isEmpty() ? idEdit->text().trimmed() : nameEdit->text().trimmed();
            QString newId, perr;
            if (!plugins().createSkeleton(idEdit->text().trimmed(), name, newId, perr, fmtBox->currentIndex() == 1 ? "xml" : "yaml", true)) {
                QMessageBox::warning(this, tr("Plugin"), perr);
                return;
            }
            rebuild();
            selectId(newId);
        }
        const QString dir = currentDir();
        if (dir.isEmpty()) return;
        QDir().mkpath(dir + "/textures");
        QJsonArray blocks;
        int failed = 0;
        for (const Pick &pk : picks) {
            const QImage img = specImage(pk.file); // base + textures par-dessus fusionnées
            const QString rel = "textures/" + QString(pk.ref).replace(':', '_') + ".png";
            if (img.isNull() || !img.save(dir + "/" + rel, "PNG")) { ++failed; continue; }
            const QString modPart = pk.ref.section(':', 0, 0), namePart = pk.ref.section(':', 1);
            QJsonObject b;
            b["ref"] = pk.ref;
            b["label"] = QString::fromStdString(lctex::prettyLabel(namePart.toStdString()));
            b["category"] = QString::fromStdString(lctex::prettyLabel(modPart.toStdString()));
            b["texture"] = rel;
            blocks.append(b);
        }
        const bool ok = editJson([&](QJsonObject &o) {
            if (!intoCurrent) {
                o["description"] = tr("Blocs et textures du mod « %1 ».").arg(QString::fromStdString(modName));
                o["blocks"] = blocks;
                return;
            }
            // plugin existant : on ajoute les blocs ; un bloc déjà déclaré avec la même référence est mis à jour
            QJsonArray a = o["blocks"].toArray();
            for (const QJsonValue &nb : blocks) {
                bool replaced = false;
                for (int i = 0; i < a.size(); ++i)
                    if (a[i].toObject().value("ref").toString() == nb.toObject().value("ref").toString()) { a[i] = nb; replaced = true; break; }
                if (!replaced) a.append(nb);
            }
            o["blocks"] = a;
        });
        if (ok) {
            QString msg = intoCurrent ? tr("%1 bloc(s) importé(s) du mod « %2 ».").arg(blocks.size()).arg(QString::fromStdString(modName))
                                      : tr("Plugin créé : %1 bloc(s).").arg(blocks.size());
            if (failed) msg += tr(" %1 texture(s) illisible(s) ignorée(s).").arg(failed);
            afterEdit(msg);
        }
    }

    void exportPlugin()
    {
        const Plugin *p = current();
        if (!p) return;
        const QString zp = QFileDialog::getSaveFileName(this, tr("Exporter le plugin"), QDir::homePath() + "/" + p->id + ".zip",
                                                        tr("Archive zip (*.zip)"));
        if (zp.isEmpty()) return;
        QString err;
        if (plugins().exportZip(p->id, zp, err)) QMessageBox::information(this, tr("Plugin"), tr("Archive créée : %1").arg(zp));
        else QMessageBox::warning(this, tr("Plugin"), err);
    }

    void removePlugin()
    {
        const Plugin *p = current();
        if (!p) return;
        if (QMessageBox::question(this, tr("Supprimer"), tr("Supprimer le plugin « %1 » et ses fichiers ?").arg(p->name)) !=
            QMessageBox::Yes)
            return;
        plugins().remove(p->id);
        rebuild();
        changed();
    }

    // ------------------------------------------------------------ éditeur
    void loadFileList()
    {
        files_->blockSignals(true);
        files_->clear();
        const QString d = currentDir();
        edTitle_->setText(d.isEmpty() ? tr("Plugin introuvable (supprimé ?).") : tr("Plugin : %1").arg(d));
        if (!d.isEmpty()) {
            QStringList all;
            QDirIterator it(d, QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                const QString rel = QDir(d).relativeFilePath(it.next());
                if (rel.endsWith(".png", Qt::CaseInsensitive)) continue;
                all << rel;
            }
            all.sort();
            const QString mf = lcman::find(d);
            if (!mf.isEmpty()) {
                all.removeAll(mf);
                all.prepend(mf);
            }
            files_->addItems(all);
        }
        files_->blockSignals(false);
        openFile(files_->currentText());
    }

    void openFile(const QString &rel)
    {
        const QString d = currentDir();
        editor_->setEnabled(!d.isEmpty() && !rel.isEmpty());
        if (d.isEmpty() || rel.isEmpty()) {
            if (editorHl_) editorHl_->setLang(LcLang::None, false);
            editor_->clear();
            return;
        }
        QFile f(d + "/" + rel);
        if (editorHl_) editorHl_->setLang(lcLangForFile(rel), false); // avant setPlainText : une seule coloration
        editor_->setPlainText(f.open(QIODevice::ReadOnly) ? QString::fromUtf8(f.readAll()) : QString());
        editor_->document()->setModified(false);
        status_->clear();
    }

    void saveFile()
    {
        const QString d = currentDir(), rel = files_->currentText();
        if (d.isEmpty() || rel.isEmpty()) return;
        rememberTrust();
        if (lcman::isManifest(rel)) { // le manifeste doit rester lisible : on refuse d'enregistrer une erreur de syntaxe
            QJsonObject test;
            QString perr;
            if (!lcman::parse(editor_->toPlainText().toUtf8(), rel, test, perr)) {
                status_->setText(tr("<span style='color:#e53935'>Non enregistré : %1</span>").arg(perr.toHtmlEscaped()));
                return;
            }
        }
        QFile f(d + "/" + rel);
        if (!f.open(QIODevice::WriteOnly)) {
            status_->setText(tr("Impossible d'écrire %1").arg(rel));
            return;
        }
        f.write(editor_->toPlainText().toUtf8());
        f.close();
        editor_->document()->setModified(false);
        afterEdit(tr("Enregistré."));
    }

    // recharge les plugins et affiche les problèmes éventuels
    void afterEdit(const QString &okMsg)
    {
        const QString id = currentId(), file = files_ ? files_->currentText() : QString();
        plugins().reload();
        if (trust_) {
            if (const Plugin *tp = plugins().find(id)) plugins().setScriptsAllowed(*tp, true);
            trust_ = false;
        }
        if (trustCore_) {
            if (const Plugin *tp = plugins().find(id)) plugins().setCoreAllowed(*tp, true);
            trustCore_ = false;
        }
        rebuild();
        selectId(id);
        if (files_) {
            const int idx = files_->findText(file);
            if (idx >= 0) files_->setCurrentIndex(idx);
        }
        changed();
        reloadWholeApp(); // un plugin enregistré / validé : toute l'application est rechargée
        const Plugin *p = plugins().find(id);
        if (!status_) { // gestionnaire : pas de zone d'état, on informe par une boîte de dialogue
            QMessageBox::information(this, tr("Plugin"),
                                     okMsg + (p && !p->problems.isEmpty() ? "\n\n" + tr("Problèmes :") + "\n- " + p->problems.join("\n- ") : QString()));
            return;
        }
        if (p && !p->problems.isEmpty())
            status_->setText(okMsg + " <span style='color:#e08a00'>" + tr("Problèmes : ") +
                             p->problems.join(" • ").toHtmlEscaped() + "</span>");
        else
            status_->setText(okMsg + tr(" Aucun problème détecté."));
    }

    void validate()
    {
        const QString id = currentId();
        if (id.isEmpty()) return;
        plugins().reload();
        rebuild();
        const Plugin *p = plugins().find(id);
        if (!p) return;
        status_->setText(p->problems.isEmpty() ? tr("Le plugin est valide.")
                                               : "<span style='color:#e08a00'>" + p->problems.join(" • ").toHtmlEscaped() + "</span>");
        changed();
        reloadWholeApp();
    }

    void newFile()
    {
        const QString d = currentDir();
        if (d.isEmpty()) return;
        rememberTrust();
        bool ok = false;
        const QString rel = QInputDialog::getText(this, tr("Nouveau fichier"),
                                                  tr("Chemin dans le plugin (ex : lua/mon_extrait.lua, templates/x.lua ou scripts/gen.py) :"),
                                                  QLineEdit::Normal, "lua/nouveau.lua", &ok).trimmed();
        if (!ok || rel.isEmpty()) return;
        if (!PluginHost::allowedFile(rel) || rel.endsWith(".png", Qt::CaseInsensitive)) {
            QMessageBox::warning(this, tr("Fichier"), tr("Nom non autorisé (types acceptés : .lua .tpl .txt .md .yaml .xml .py .sh .js .rb .pl)."));
            return;
        }
        const QString path = d + "/" + rel;
        if (QFile::exists(path)) {
            QMessageBox::warning(this, tr("Fichier"), tr("Ce fichier existe déjà."));
            return;
        }
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile f(path);
        if (f.open(QIODevice::WriteOnly)) f.write("-- \n");
        f.close();
        loadFileList();
        files_->setCurrentText(rel);
    }

    // copie une image PNG dans le plugin ; renvoie le chemin relatif ou une chaîne vide
    QString copyImage(const QString &src, const QString &subdir, const QString &baseName)
    {
        const QString d = currentDir();
        if (d.isEmpty() || QImage(src).isNull()) {
            QMessageBox::warning(this, tr("Image"), tr("Image illisible : %1").arg(src));
            return QString();
        }
        const QString rel = subdir + "/" + baseName + ".png";
        QDir().mkpath(d + "/" + subdir);
        QFile::remove(d + "/" + rel);
        // on réencode en PNG : seul ce format est accepté dans un plugin
        if (!QImage(src).save(d + "/" + rel, "PNG")) {
            QMessageBox::warning(this, tr("Image"), tr("Impossible d'écrire %1").arg(rel));
            return QString();
        }
        return rel;
    }

    void importImage()
    {
        if (currentDir().isEmpty()) return;
        const QString src = QFileDialog::getOpenFileName(this, tr("Image"), QString(), tr("Images (*.png *.jpg *.jpeg *.bmp)"));
        if (src.isEmpty()) return;
        const QString rel = copyImage(src, "textures", QFileInfo(src).completeBaseName());
        if (!rel.isEmpty()) afterEdit(tr("Image copiée dans %1.").arg(rel));
    }

    // lit le manifeste (yaml / xml) en objet, le modifie, puis le réécrit dans le même format
    bool editJson(const std::function<void(QJsonObject &)> &fn)
    {
        const QString mf = lcman::find(currentDir());
        if (mf.isEmpty()) return false;
        rememberTrust();
        const QString path = currentDir() + "/" + mf;
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly)) return false;
        QJsonObject o;
        QString perr;
        const bool ok = lcman::parse(f.readAll(), mf, o, perr);
        f.close();
        if (!ok) {
            QMessageBox::warning(this, mf, tr("%1\nCorrigez d'abord le fichier (onglet « Coder un plugin »).").arg(perr));
            return false;
        }
        fn(o);
        QFile w(path);
        if (!w.open(QIODevice::WriteOnly)) return false;
        w.write(lcman::serialize(o, mf)); // les commentaires du fichier ne sont pas conservés lors d'une modification par les boutons
        return true;
    }

    // Importe plusieurs textures et devine « mod:bloc » d'après leurs noms (voir texturedetect.h).
    // Plus on importe de fichiers à la fois, plus la détection est fiable (le préfixe qui se répète est le mod).
    void importTextures()
    {
        const Plugin *pl = current();
        if (!pl || currentDir().isEmpty()) return;
        const QString defaultCategory = pl->name;
        LcDialog dlg(this);
        dlg.setWindowTitle(tr("Importer des textures"));
        UiHooks::bindWindow(&dlg, "textures");
        dlg.resize(980, 600);
        auto *v = new QVBoxLayout(&dlg);
        auto *help = new QLabel(tr("Sélectionnez plusieurs textures : le mod et le nom du bloc sont devinés d'après les noms de fichiers "
                                   "(<code>default_dirt.png</code> → <code>default:dirt</code>). Avec un seul fichier la détection peut se tromper ; "
                                   "avec plusieurs, ce qui se répète donne le nom du mod. Les faces (<code>_top</code>, <code>_side</code>…) "
                                   "sont regroupées sur le même bloc : la texture de face (<code>_front</code>, puis <code>_side</code>) est préférée, et un "
                                   "<b>double-clic sur la case du fichier</b> ouvre en dessous les textures du bloc pour choisir celle à utiliser. "
                                   "Corrigez les autres cellules si besoin (double-clic)."));
        help->setWordWrap(true);
        help->setTextFormat(Qt::RichText);
        v->addWidget(help);

        auto *bar = new QHBoxLayout;
        auto *bFiles = new QPushButton(tr("Ajouter des images..."));
        UiHooks::bind(bFiles, "btn.addimages", "dialog.textures");
        auto *bDir = new QPushButton(tr("Ajouter un dossier..."));
        UiHooks::bind(bDir, "btn.adddir", "dialog.textures");
        auto *bClear = new QPushButton(tr("Vider la liste"));
        UiHooks::bind(bClear, "btn.clear", "dialog.textures");
        bar->addWidget(bFiles);
        bar->addWidget(bDir);
        bar->addWidget(bClear);
        bar->addStretch(1);
        v->addLayout(bar);

        auto *table = new QTableWidget(0, 6);
        table->setHorizontalHeaderLabels({tr("Fichier"), tr("Mod"), tr("Bloc"), tr("Nom affiché"), tr("Catégorie"), tr("Détection")});
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setIconSize(QSize(24, 24));
        table->horizontalHeader()->setStretchLastSection(true);
        table->setColumnWidth(0, 260);
        table->setColumnWidth(1, 130);
        table->setColumnWidth(2, 160);
        table->setColumnWidth(3, 150);
        table->setColumnWidth(4, 110);
        v->addWidget(table, 1);

        // panneau « textures du bloc » (double-clic sur la case du fichier)
        auto *panel = new QWidget;
        auto *pv = new QVBoxLayout(panel);
        pv->setContentsMargins(0, 0, 0, 0);
        auto *panelLbl = new QLabel;
        panelLbl->setWordWrap(true);
        auto *varList = new QListWidget;
        varList->setViewMode(QListView::IconMode);
        varList->setIconSize(QSize(48, 48));
        varList->setResizeMode(QListView::Adjust);
        varList->setMovement(QListView::Static);
        varList->setWrapping(true);
        varList->setFixedHeight(116);
        pv->addWidget(panelLbl);
        pv->addWidget(varList);
        panel->hide();
        v->addWidget(panel);

        auto *summary = new QLabel;
        summary->setWordWrap(true);
        v->addWidget(summary);

        auto *opts = new QFormLayout;
        auto *forceMod = new QLineEdit;
        forceMod->setPlaceholderText(tr("ex. default (laisser vide pour la détection automatique)"));
        auto *bForce = new QPushButton(tr("Appliquer à toutes les lignes"));
        UiHooks::bind(bForce, "btn.applyall", "dialog.textures");
        auto *rowF = new QHBoxLayout;
        rowF->addWidget(forceMod, 1);
        rowF->addWidget(bForce);
        auto *defMod = new QLineEdit(identOf(pl->id));
        opts->addRow(tr("Imposer le mod :"), rowF);
        opts->addRow(tr("Mod des fichiers sans préfixe (ex. dirt.png) :"), defMod);
        v->addLayout(opts);

        auto *bb = new QDialogButtonBox(QDialogButtonBox::Cancel);
        auto *bOk = bb->addButton(tr("Importer les textures cochées"), QDialogButtonBox::AcceptRole);
        UiHooks::bindBox(bb, "textures");
        v->addWidget(bb);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        QStringList paths;
        const int RoleFull = Qt::UserRole, RoleConf = Qt::UserRole + 1, RoleVariant = Qt::UserRole + 2, RoleText = Qt::UserRole + 3;
        const auto refOfRow = [&](int r) { return table->item(r, 1)->text() + ":" + table->item(r, 2)->text(); };

        // Une seule texture est utilisée par bloc : `chosen` est cochée, les autres (faces) sont décochées.
        auto markRef = [&](const QString &ref, int chosen) {
            for (int r = 0; r < table->rowCount(); ++r) {
                if (refOfRow(r) != ref) continue;
                const QString var = table->item(r, 0)->data(RoleVariant).toString();
                table->item(r, 0)->setCheckState(r == chosen ? Qt::Checked : Qt::Unchecked);
                QString txt = table->item(r, 5)->data(RoleText).toString();
                if (!var.isEmpty())
                    txt += r == chosen ? tr(" — face « %1 » utilisée").arg(var) : tr(" — autre face « %1 » (double-clic pour choisir)").arg(var);
                else if (r != chosen) txt += tr(" — non utilisée (double-clic pour choisir)");
                table->item(r, 5)->setText(txt);
            }
        };

        auto refill = [&] {
            QSignalBlocker blocker(table);
            panel->hide();
            varList->clear();
            table->setRowCount(0);
            std::vector<std::string> names;
            for (const QString &p : paths) names.push_back(QFileInfo(p).fileName().toStdString());
            const std::vector<lctex::Detected> det = lctex::detect(names);
            QMap<QString, QList<int>> byRef;
            QMap<QString, int> modCount;
            for (size_t i = 0; i < det.size(); ++i) {
                const lctex::Detected &d = det[i];
                QString mod = QString::fromStdString(d.mod), name = QString::fromStdString(d.name);
                const QString full = mod.isEmpty() ? name : mod + "_" + name;
                if (mod.isEmpty()) mod = identOf(defMod->text());
                else modCount[mod] += 1;
                const int r = table->rowCount();
                table->insertRow(r);
                auto *fi = new QTableWidgetItem(QIcon(QPixmap(paths[(int)i]).scaled(24, 24, Qt::KeepAspectRatio, Qt::FastTransformation)),
                                                QFileInfo(paths[(int)i]).fileName());
                fi->setFlags((fi->flags() | Qt::ItemIsUserCheckable) & ~Qt::ItemIsEditable);
                fi->setCheckState(Qt::Unchecked);
                fi->setData(RoleFull, full);
                fi->setData(RoleConf, (int)d.conf);
                fi->setData(RoleVariant, QString::fromStdString(d.variant));
                table->setItem(r, 0, fi);
                table->setItem(r, 1, new QTableWidgetItem(mod));
                table->setItem(r, 2, new QTableWidgetItem(name));
                table->setItem(r, 3, new QTableWidgetItem(QString::fromStdString(lctex::prettyLabel(d.name))));
                table->setItem(r, 4, new QTableWidgetItem(mod.isEmpty() ? defaultCategory : QString::fromStdString(lctex::prettyLabel(mod.toStdString()))));
                QString txt;
                QColor col;
                switch (d.conf) {
                case lctex::Confidence::Sure: txt = tr("● sûr"); col = QColor("#2e9d4f"); break;
                case lctex::Confidence::Probable: txt = tr("● probable (2 fichiers)"); col = QColor("#d08a00"); break;
                case lctex::Confidence::Unsure: txt = tr("● incertain (fichier seul)"); col = QColor("#d04040"); break;
                case lctex::Confidence::NoMod: txt = tr("○ pas de préfixe : mod par défaut"); col = QColor("#888888"); break;
                }
                auto *ci = new QTableWidgetItem(txt);
                ci->setData(RoleText, txt);
                ci->setFlags(ci->flags() & ~Qt::ItemIsEditable);
                ci->setForeground(col);
                table->setItem(r, 5, ci);
                byRef[mod + ":" + name] << r;
            }
            // pour chaque bloc : la texture de face (front, puis side) est préférée, puis la texture sans suffixe, puis le dessus
            for (auto it = byRef.begin(); it != byRef.end(); ++it) {
                int pick = it.value().first(), best = 99;
                for (int r : it.value()) {
                    const int rank = lctex::variantRank(table->item(r, 0)->data(RoleVariant).toString().toStdString());
                    if (rank < best) { best = rank; pick = r; }
                }
                markRef(it.key(), pick);
            }
            if (paths.isEmpty()) {
                summary->setText(tr("Aucune texture."));
            } else {
                QStringList parts;
                for (auto it = modCount.begin(); it != modCount.end(); ++it) parts << QString("<b>%1</b> (%2)").arg(it.key().toHtmlEscaped()).arg(it.value());
                summary->setText(tr("%1 fichier(s). ").arg(paths.size()) +
                                 (parts.isEmpty() ? tr("Aucun mod détecté : le mod par défaut sera utilisé.") : tr("Mod(s) détecté(s) : %1.").arg(parts.join(", "))));
            }
            bOk->setEnabled(!paths.isEmpty());
        };

        auto addPaths = [&](const QStringList &list) {
            for (const QString &p : list)
                if (!paths.contains(p) && !QImage(p).isNull()) paths << p;
            refill();
        };
        connect(bFiles, &QPushButton::clicked, &dlg, [&] {
            addPaths(QFileDialog::getOpenFileNames(&dlg, tr("Textures"), QString(), tr("Images (*.png *.jpg *.jpeg *.bmp)")));
        });
        connect(bDir, &QPushButton::clicked, &dlg, [&] {
            const QString d = QFileDialog::getExistingDirectory(&dlg, tr("Dossier de textures"));
            if (d.isEmpty()) return;
            QStringList found;
            QDirIterator it(d, QStringList() << "*.png" << "*.jpg" << "*.jpeg" << "*.bmp", QDir::Files, QDirIterator::Subdirectories);
            while (it.hasNext() && found.size() < 2000) found << it.next();
            found.sort();
            addPaths(found);
        });
        connect(bClear, &QPushButton::clicked, &dlg, [&] {
            paths.clear();
            refill();
        });
        // mod imposé : le préfixe « mod_ » est retiré du nom s'il y est, sinon le nom complet du fichier est gardé
        connect(bForce, &QPushButton::clicked, &dlg, [&] {
            const QString m = identOf(forceMod->text());
            if (m.isEmpty()) return;
            QSignalBlocker blocker(table);
            for (int r = 0; r < table->rowCount(); ++r) {
                const QString full = table->item(r, 0)->data(RoleFull).toString();
                const QString pre = m + "_";
                table->item(r, 1)->setText(m);
                table->item(r, 2)->setText(full.startsWith(pre) && full.size() > pre.size() ? full.mid(pre.size()) : full);
                table->item(r, 4)->setText(QString::fromStdString(lctex::prettyLabel(m.toStdString())));
            }
        });
        connect(defMod, &QLineEdit::editingFinished, &dlg, [&] {
            QSignalBlocker blocker(table);
            for (int r = 0; r < table->rowCount(); ++r)
                if (table->item(r, 0)->data(RoleConf).toInt() == (int)lctex::Confidence::NoMod) table->item(r, 1)->setText(identOf(defMod->text()));
        });
        auto showVariants = [&](int row) {
            if (row < 0 || row >= table->rowCount()) return;
            const QString ref = refOfRow(row);
            varList->clear();
            for (int r = 0; r < table->rowCount(); ++r) {
                if (refOfRow(r) != ref) continue;
                const QString var = table->item(r, 0)->data(RoleVariant).toString();
                auto *it = new QListWidgetItem(QIcon(QPixmap(paths[r]).scaled(48, 48, Qt::KeepAspectRatio, Qt::FastTransformation)),
                                               var.isEmpty() ? tr("principale") : var);
                it->setToolTip(QFileInfo(paths[r]).fileName());
                it->setData(Qt::UserRole, r);
                varList->addItem(it);
                if (table->item(r, 0)->checkState() == Qt::Checked) varList->setCurrentItem(it);
            }
            panelLbl->setText(tr("Textures du bloc <b>%1</b> — cliquez sur celle à utiliser :").arg(ref.toHtmlEscaped()));
            panel->show();
        };
        connect(table, &QTableWidget::cellDoubleClicked, &dlg, [&](int r, int c) {
            if (c == 0) showVariants(r);
        });
        connect(varList, &QListWidget::itemClicked, &dlg, [&](QListWidgetItem *it) {
            const int r = it->data(Qt::UserRole).toInt();
            if (r >= 0 && r < table->rowCount()) markRef(refOfRow(r), r);
        });
        refill();
        QTimer::singleShot(0, &dlg, [&] { bFiles->click(); });
        if (dlg.exec() != QDialog::Accepted) return;

        struct Row { QString mod, name, label, cat, rel; };
        QVector<Row> rows;
        QSet<QString> seen;
        int skipped = 0, failed = 0;
        for (int r = 0; r < table->rowCount(); ++r) {
            if (table->item(r, 0)->checkState() != Qt::Checked) continue;
            Row w;
            w.mod = identOf(table->item(r, 1)->text());
            w.name = identOf(table->item(r, 2)->text());
            if (w.mod.isEmpty() || w.name.isEmpty()) { ++failed; continue; }
            const QString ref = w.mod + ":" + w.name;
            if (seen.contains(ref)) { ++skipped; continue; } // deux fichiers pour le même bloc : le premier coché est gardé
            w.label = table->item(r, 3)->text().trimmed();
            w.cat = table->item(r, 4)->text().trimmed();
            w.rel = copyImage(paths[r], "textures", w.mod + "_" + w.name);
            if (w.rel.isEmpty()) { ++failed; continue; }
            seen.insert(ref);
            rows << w;
        }
        if (rows.isEmpty()) return;
        const bool ok = editJson([&](QJsonObject &o) {
            QJsonArray a = o["blocks"].toArray();
            for (const Row &w : rows) {
                QJsonObject b;
                b["ref"] = w.mod + ":" + w.name;
                if (!w.label.isEmpty()) b["label"] = w.label;
                if (!w.cat.isEmpty()) b["category"] = w.cat;
                b["texture"] = w.rel;
                bool replaced = false; // un bloc déjà déclaré avec la même référence est mis à jour
                for (int i = 0; i < a.size(); ++i)
                    if (a[i].toObject().value("ref").toString() == b["ref"].toString()) { a[i] = b; replaced = true; break; }
                if (!replaced) a.append(b);
            }
            o["blocks"] = a;
        });
        if (ok) {
            QString msg = tr("%1 bloc(s) importé(s).").arg(rows.size());
            if (skipped) msg += tr(" %1 doublon(s) ignoré(s).").arg(skipped);
            if (failed) msg += tr(" %1 fichier(s) ignoré(s) (nom ou image invalide).").arg(failed);
            afterEdit(msg);
        }
    }

    void addBlock()
    {
        const Plugin *pl = current();
        if (!pl) return;
        LcDialog dlg(this);
        dlg.setWindowTitle(tr("Ajouter un bloc au plugin"));
        UiHooks::bindWindow(&dlg, "addblock");
        auto *form = new QFormLayout(&dlg);
        auto *ref = new QLineEdit;
        ref->setPlaceholderText("wool:blue");
        auto *label = new QLineEdit;
        auto *cat = new QComboBox;
        cat->setEditable(true);
        QSet<QString> cats;
        for (const PluginBlock &b : pl->blocks) cats.insert(b.category);
        cat->addItems(QStringList(cats.begin(), cats.end()));
        if (cats.isEmpty()) cat->setCurrentText(pl->name);
        auto *tex = new QLineEdit;
        auto *bt = new QPushButton("...");
        UiHooks::bind(bt, "btn.pick", "dialog.addblock");
        auto *row = new QHBoxLayout;
        row->addWidget(tex, 1);
        row->addWidget(bt);
        connect(bt, &QPushButton::clicked, &dlg, [&] {
            const QString p = QFileDialog::getOpenFileName(&dlg, tr("Texture"), QString(), tr("Images (*.png *.jpg *.bmp)"));
            if (!p.isEmpty()) tex->setText(p);
        });
        form->addRow(tr("Référence (mod:nom)"), ref);
        form->addRow(tr("Nom affiché"), label);
        form->addRow(tr("Catégorie"), cat);
        form->addRow(tr("Texture (optionnel)"), row);
        auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        UiHooks::bindBox(bb, "addblock");
        form->addRow(bb);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        if (dlg.exec() != QDialog::Accepted) return;
        const QString r = ref->text().trimmed();
        if (!r.contains(':') || r.startsWith(':') || r.endsWith(':')) {
            QMessageBox::warning(this, tr("Bloc"), tr("La référence doit être de la forme mod:nom."));
            return;
        }
        QString texRel;
        if (!tex->text().trimmed().isEmpty()) {
            texRel = copyImage(tex->text().trimmed(), "textures", identOf(r));
            if (texRel.isEmpty()) return;
        }
        const bool ok = editJson([&](QJsonObject &o) {
            QJsonArray a = o["blocks"].toArray();
            QJsonObject b;
            b["ref"] = r;
            if (!label->text().trimmed().isEmpty()) b["label"] = label->text().trimmed();
            if (!cat->currentText().trimmed().isEmpty()) b["category"] = cat->currentText().trimmed();
            if (!texRel.isEmpty()) b["texture"] = texRel;
            a.append(b);
            o["blocks"] = a;
        });
        if (ok) afterEdit(tr("Bloc ajouté."));
    }

    void addIcon()
    {
        const Plugin *plug = current();
        if (!plug) return;
        LcDialog dlg(this);
        dlg.setWindowTitle(tr("Ajouter une icône au plugin"));
        UiHooks::bindWindow(&dlg, "addicon");
        dlg.resize(560, 640);
        auto *form = new QFormLayout(&dlg);

        // --- sélecteur de clés : un point de couleur indique l'état de chaque clé dans CE plugin
        auto *filter = new QLineEdit;
        filter->setPlaceholderText(tr("Filtrer les clés (ex : biomes, btn.add, plugin.mon_plugin...)"));
        auto *keys = new QListWidget;
        keys->setMinimumHeight(240);
        const Plugin snapshot = *plug; // l'état est calculé sur le plugin tel qu'il est à l'ouverture
        QStringList own; // clés des pages de ce plugin en premier
        for (const PluginFeature &f : snapshot.features) own << "card." + f.key;
        QStringList all = knownIconKeys(&snapshot);
        for (const QString &k : own) all.removeAll(k);
        all = own + all;
        for (const QString &k : all) {
            QString via;
            const IconState st = iconKeyState(snapshot, k, &via);
            auto *it = new QListWidgetItem(iconStateDot(st), k);
            it->setForeground(iconStateColor(st));
            it->setToolTip(iconStateText(st, via));
            it->setData(Qt::UserRole, int(st));
            keys->addItem(it);
        }
        auto *legend = new QLabel(tr("<span style='color:#e53935'>●</span> rouge : icône non enregistrée &nbsp; "
                                     "<span style='color:#fb8c00'>●</span> orange : couverte par une clé englobante &nbsp; "
                                     "<span style='color:#43a047'>●</span> vert : icône enregistrée"));
        legend->setWordWrap(true);
        auto *key = new QLineEdit;
        key->setPlaceholderText(tr("Clé de l'icône (cliquez dans la liste ou saisissez-la)"));
        auto *state = new QLabel;
        state->setWordWrap(true);
        auto showState = [&] {
            const QString k = key->text().trimmed();
            if (k.isEmpty()) {
                state->clear();
                return;
            }
            QString via;
            const IconState st = iconKeyState(snapshot, k, &via);
            state->setText(QString("<span style='color:%1'>●</span> %2").arg(iconStateColor(st).name(), iconStateText(st, via).toHtmlEscaped()));
        };
        connect(filter, &QLineEdit::textChanged, &dlg, [keys](const QString &t) {
            for (int i = 0; i < keys->count(); ++i) keys->item(i)->setHidden(!keys->item(i)->text().contains(t.trimmed(), Qt::CaseInsensitive));
        });
        connect(keys, &QListWidget::currentItemChanged, &dlg, [key, &showState](QListWidgetItem *it, QListWidgetItem *) {
            if (it) key->setText(it->text());
            showState();
        });
        connect(key, &QLineEdit::textChanged, &dlg, [&showState](const QString &) { showState(); });
        form->addRow(filter);
        form->addRow(keys);
        form->addRow(legend);
        form->addRow(tr("Clé"), key);
        form->addRow(state);
        auto *img = new QLineEdit;
        auto *bt = new QPushButton("...");
        UiHooks::bind(bt, "btn.pick", "dialog.addicon");
        auto *row = new QHBoxLayout;
        row->addWidget(img, 1);
        row->addWidget(bt);
        connect(bt, &QPushButton::clicked, &dlg, [&] {
            const QString p = QFileDialog::getOpenFileName(&dlg, tr("Icône"), QString(), tr("Images (*.png *.jpg *.bmp)"));
            if (!p.isEmpty()) img->setText(p);
        });
        auto *help = new QLabel(tr("card.* : cartes de l'accueil — btn.* : boutons (tous) — <contexte>.btn.* : boutons d'une page précise (home, toolbar, server, editor, editor.<page>, plugins, page.<page>) — cat.* : catégories des sélecteurs de blocs "
                                   "(cat.<nom de catégorie en minuscules, a-z0-9_>) — app : icône de la fenêtre."));
        help->setWordWrap(true);
        help->setTextFormat(Qt::PlainText);
        form->addRow(tr("Image (PNG conseillé, 64×64)"), row);
        auto *only = new QCheckBox(tr("Icône seule (masquer le texte du bouton / de la carte)"));
        form->addRow(only);
        form->addRow(help);
        auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        UiHooks::bindBox(bb, "addicon");
        form->addRow(bb);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        if (dlg.exec() != QDialog::Accepted) return;
        const QString k = key->text().trimmed();
        if (k.isEmpty() || img->text().trimmed().isEmpty()) return;
        const QString rel = copyImage(img->text().trimmed(), "icons", identOf(k));
        if (rel.isEmpty()) return;
        const bool ok = editJson([&](QJsonObject &o) {
            QJsonObject ic = o["icons"].toObject();
            if (only->isChecked()) ic[k] = QJsonObject{{"file", rel}, {"icon_only", true}};
            else ic[k] = rel;
            o["icons"] = ic;
        });
        if (ok) afterEdit(tr("Icône ajoutée."));
    }

    static QString referenceMarkdown()
    {
        QTextDocument document;
        document.setHtml(referenceHtml());
        return document.toMarkdown();
    }

    static QString referenceHtml()
    {
        return QObject::tr(R"(<h2>Format d'un plugin</h2>
<p>Un plugin est un dossier (ou une archive .zip) contenant un manifeste <code>plugin.yaml</code> <b>ou</b> <code>plugin.xml</code>
et des fichiers : images PNG, extraits Lua (<code>.lua</code>), gabarits (<code>.tpl</code> ou <code>.lua</code>). Aucun code natif n'est exécuté.
Les deux formats sont équivalents ; l'ancien <code>plugin.json</code> est encore lu mais n'est plus créé.</p>
<h3>YAML (plugin.yaml)</h3>
<pre># commentaire
format: 1
id: wool_pack
name: Laines
version: "1.0"
author: Vous
description: Pack de laines et d'icônes
icons:
  card.biomes: icons/biomes.png
  btn.add: { file: icons/add.png, icon_only: true }   # icône seule : le texte est masqué
  cat.laine: icons/wool.png
textures:
  "default:stone": textures/stone.png
blocks:
  - ref: "wool:blue"
    label: Laine bleue
    category: Laine
    texture: textures/blue.png
blocksets:
  - ref: "wool:{name}"
    label: "Laine {name}"
    category: Laine
    texture: "textures/wool_{name}.png"
    names: [white, red, blue]
presets:
  - tab: biomes
    name: Prairie
    values: { node_top: "default:dirt_with_grass", heat: 50 }
snippets:
  - { id: bienvenue, name: Bienvenue, file: lua/bienvenue.lua }
features:
  - id: salutations
    title: Salutations
    icon: icons/s.png
    prefix: salut_
    template: templates/salut.lua
    fields:
      - { key: name, label: Nom, type: text }
      - { key: fois, label: Répétitions, type: int, default: 1, min: 1, max: 10 }
      - key: mode
        type: combo
        options: [a, b]</pre>
<h3>XML (plugin.xml)</h3>
<pre>&lt;?xml version="1.0" encoding="UTF-8"?&gt;
&lt;plugin format="1" id="wool_pack" name="Laines" version="1.0" author="Vous"&gt;
  &lt;description&gt;Pack de laines et d'icônes&lt;/description&gt;
  &lt;icons&gt;
    &lt;icon key="card.biomes" file="icons/biomes.png"/&gt;
    &lt;icon key="btn.add" file="icons/add.png" icon_only="true"/&gt;
  &lt;/icons&gt;
  &lt;textures&gt;&lt;texture ref="default:stone" file="textures/stone.png"/&gt;&lt;/textures&gt;
  &lt;blocks&gt;
    &lt;block ref="wool:blue" label="Laine bleue" category="Laine" texture="textures/blue.png"/&gt;
  &lt;/blocks&gt;
  &lt;blocksets&gt;
    &lt;blockset ref="wool:{name}" label="Laine {name}" category="Laine" texture="textures/wool_{name}.png"&gt;
      &lt;names&gt;&lt;name&gt;white&lt;/name&gt;&lt;name&gt;red&lt;/name&gt;&lt;/names&gt;
    &lt;/blockset&gt;
  &lt;/blocksets&gt;
  &lt;presets&gt;
    &lt;preset tab="biomes" name="Prairie"&gt;
      &lt;values&gt;&lt;value key="node_top"&gt;default:dirt_with_grass&lt;/value&gt;&lt;value key="heat"&gt;50&lt;/value&gt;&lt;/values&gt;
    &lt;/preset&gt;
  &lt;/presets&gt;
  &lt;snippets&gt;&lt;snippet id="bienvenue" name="Bienvenue" file="lua/bienvenue.lua"/&gt;&lt;/snippets&gt;
  &lt;features&gt;
    &lt;feature id="salutations" title="Salutations" prefix="salut_" template="templates/salut.lua"&gt;
      &lt;fields&gt;
        &lt;field key="name" label="Nom" type="text"/&gt;
        &lt;field key="mode" type="combo"&gt;&lt;options&gt;&lt;option&gt;a&lt;/option&gt;&lt;option&gt;b&lt;/option&gt;&lt;/options&gt;&lt;/field&gt;
      &lt;/fields&gt;
    &lt;/feature&gt;
  &lt;/features&gt;
&lt;/plugin&gt;</pre>
<p>XML : les listes (<code>blocks blocksets presets snippets features fields names options</code>) et les tables
(<code>icons textures values</code>) s'écrivent avec leur élément conteneur. Les attributs <code>icon-only</code> et <code>icon_only</code> sont équivalents.</p>
<p><b>Icône seule :</b> <code>icon_only: true</code> masque le texte du bouton (ou le titre et la description de la carte) ;
les boutons d'un groupe qui ont tous une icône se placent sur une même ligne.</p>
<h3>Icônes</h3>
<p><b>app</b> (icône par défaut des fenêtres) — <b>window.&lt;fenêtre&gt;</b> (launcher editor pluginedit plugins apidoc et boîtes de dialogue picker export newmod newplugin frommod textures addblock addicon) — <b>menu.&lt;action&gt;</b> (actions des menus) — <b>dialog.&lt;boîte&gt;.btn.ok|cancel|accept</b> — <b>card.&lt;page&gt;</b> : project nodes models items foods tools crafts ores biomes decos entities cmds pixel formspec lua server folder export save open plugins
et <code>card.plugin.&lt;plugin&gt;.&lt;fonction&gt;</code> — <b>btn.&lt;action&gt;</b> : add dup del pick start restart stop join reset openworld zip folder back test install new export reload open import edit about save check newfile image textures modimport block icon browse detect clearlog presets run color clear apidoc prev next selectbox refresh addimages adddir applyall addtext copy ok cancel accept — <b>launcher.btn.*</b> (fenêtre des projets), <b>pluginedit.btn.*</b> (éditeur de plugin), <b>plugins.btn.*</b> (gestionnaire), <b>tab.projects / tab.plugins / tab.settings</b> (onglets) —
<b>cat.&lt;catégorie&gt;</b> : mon_mod terrain pierre bois minerais verre objets nourriture, ou le nom (a-z0-9_) d'une catégorie de vos blocs.</p>
<p>Dans « Icône... » du plugin, chaque clé est précédée d'un point de couleur :
<span style="color:#e53935">●</span> <b>rouge</b> = icône non enregistrée,
<span style="color:#fb8c00">●</span> <b>orange</b> = pas d'icône pour cette clé mais une clé englobante en a une (<code>btn.add</code> couvre <code>editor.nodes.btn.add</code>),
<span style="color:#43a047">●</span> <b>vert</b> = icône enregistrée. Les clés des pages que votre plugin ajoute (<code>card.plugin.&lt;plugin&gt;.&lt;fonction&gt;</code>,
<code>editor.plugin.&lt;plugin&gt;.&lt;fonction&gt;.btn.add</code>, <code>page.plugin.&lt;plugin&gt;.&lt;fonction&gt;.btn.back</code>) sont proposées en tête de liste
et rappelées dans la fiche du plugin.</p>
<h3>Blocs et textures</h3>
<p>Les blocs apparaissent dans tous les sélecteurs de blocs (biomes, minerais, décorations, recettes...) avec leur texture :
vous n'êtes plus limité à <code>default:dirt</code>. Si la référence utilise un autre mod (<code>wool:blue</code>), ce mod est ajouté
automatiquement aux dépendances du mod créé. <code>textures</code> remplace l'image d'aperçu d'une référence existante.</p>
<h3>Modèles prédéfinis</h3>
<p><code>tab</code> : nodes models items foods tools crafts ores biomes decos entities cmds, ou l'id d'une fonctionnalité du plugin.
Les clés de <code>values</code> sont celles des champs de la page.</p>
<h3>Extraits Lua</h3>
<p>Du Lua ajouté à la fin de init.lua quand vous cochez l'extrait (onglet « Extraits Lua du projet »). Avec <code>"template": true</code>,
le moteur de gabarits est appliqué (<code>{{modname}}</code>).</p>
<h3>Fonctionnalités</h3>
<p>Une page d'édition complète (liste, formulaire) avec sa carte sur l'accueil. Types de champs : text int double combo (options) file check multi ref reflist.
Le gabarit est exécuté pour chaque élément. Syntaxe : <code>{{champ}}</code> brut, <code>{{champ|lua}}</code> chaîne Lua, <code>{{champ|ident}}</code>,
<code>{{champ|num}}</code>, <code>{{champ|bool}}</code>, <code>{{champ|ref}}</code>, <code>{{champ|upper}}</code> ; blocs
<code>{{#if champ}}…{{#else}}…{{/if}}</code>, <code>{{#unless champ}}</code>, <code>{{#each champ}}{{this}}{{@index}}{{/each}}</code>.
Variables prédéfinies : <code>modname</code>, <code>name</code>, <code>name_raw</code>, <code>id</code> (mod:nom), <code>index</code>.
Pour écrire une table Lua imbriquée, séparez les accolades : <code>{ {</code>.</p>
<h3>Modifier LuantiCreator depuis Python (avancé)</h3>
<p>Un script Python peut faire un second import, <code>import luanti_creator_core as core</code>, pour modifier des parties existantes de
LuantiCreator : <code>core.set_window_title</code>, <code>core.set_card</code> / <code>core.hide_card</code>, <code>core.set_icon</code>,
<code>core.set_stylesheet</code> / <code>core.add_stylesheet</code>, <code>core.set_theme_colors</code>, <code>core.set_font</code> (événements <code>startup</code> et <code>manual</code>), <code>core.lua_prepend</code> / <code>lua_append</code> /
<code>lua_replace</code> (événements <code>generate</code> et fonctionnalités). Le manifeste doit déclarer <code>core: true</code> et
l'utilisateur doit cocher « Autoriser ce plugin à modifier LuantiCreator ». <b>Avertissement :</b> LuantiCreator affiche alors en permanence
que son comportement peut être altéré par un plugin. Valider ou enregistrer un plugin recharge toute l'application. Voir PLUGINS.md.</p>)");
    }

    PluginPageMode mode_ = PluginPageMode::Project;
    QString curId_;
    QListWidget *list_ = nullptr, *snippets_ = nullptr, *scripts_ = nullptr;
    QCheckBox *allowScripts_ = nullptr, *allowCore_ = nullptr;
    QPlainTextEdit *scriptOut_ = nullptr;
    bool trust_ = false, trustCore_ = false;
    LcDocPane *info_ = nullptr;
    QPlainTextEdit *snippetInfo_ = nullptr, *editor_ = nullptr;
    LcHighlighter *editorHl_ = nullptr;
    QComboBox *files_ = nullptr;
    QLabel *edTitle_ = nullptr, *status_ = nullptr;
    QStringList enabledSnippets_;
    bool building_ = false;
};

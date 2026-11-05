#pragma once
// Serveur Luanti de test : préparation d'un monde jetable, lancement du serveur,
// lancement du client Luanti connecté à ce serveur, console de logs.

#include <QtWidgets>
#include <functional>
#include "fsutil.h"
#include "corepack.h"
#include "uihooks.h"

class ServerPage : public QWidget {
public:
    enum State { Stopped = 0, Starting = 1, Online = 2 };

    // Fournis par la fenêtre principale :
    // met à jour le dossier local du mod et renvoie son chemin (modDir, avertissements, erreur)
    std::function<bool(QString &, QStringList &, QString &)> buildMod;
    std::function<QString()> modId;
    std::function<QStringList()> modDeps;
    std::function<void(const QString &, int)> onStatus;

    ServerPage(QWidget *parent = nullptr) : QWidget(parent)
    {
        QSettings st("LuantiCreator", "LuantiCreator");
        auto *root = new QVBoxLayout(this);

        // ---------------------------------------------------------- configuration
        auto *box = new QGroupBox(tr("Configuration du serveur de test"));
        auto *form = new QFormLayout(box);

        exeBox_ = new QComboBox;
        exeBox_->setEditable(true);
        auto *browse = new QPushButton(tr("Parcourir..."));
        UiHooks::bind(browse, "btn.browse", "server");
        auto *detect = new QPushButton(tr("Détecter"));
        UiHooks::bind(detect, "btn.detect", "server");
        auto *exeRow = new QWidget;
        auto *eh = new QHBoxLayout(exeRow);
        eh->setContentsMargins(0, 0, 0, 0);
        eh->addWidget(exeBox_, 1);
        eh->addWidget(browse);
        eh->addWidget(detect);
        form->addRow(tr("Exécutable Luanti"), exeRow);

        gameBox_ = new QComboBox;
        gameBox_->setEditable(true);
        form->addRow(tr("Jeu (gameid)"), gameBox_);

        worldEdit_ = new QLineEdit(st.value("world", "creator_test").toString());
        form->addRow(tr("Nom du monde de test"), worldEdit_);

        portSpin_ = new QSpinBox;
        portSpin_->setRange(1024, 65535);
        portSpin_->setValue(st.value("port", 30000).toInt());
        form->addRow(tr("Port"), portSpin_);

        nameEdit_ = new QLineEdit(st.value("player", "testeur").toString());
        form->addRow(tr("Nom du joueur (administrateur)"), nameEdit_);

        creative_ = new QCheckBox(tr("Mode créatif (tous les objets dans l'inventaire)"));
        creative_->setChecked(st.value("creative", true).toBool());
        damage_ = new QCheckBox(tr("Dégâts activés"));
        damage_->setChecked(st.value("damage", false).toBool());
        coreBox_ = new QCheckBox(tr("Installer « Luanti Creator Core » dans le monde (/status, /lc, /lc admin, /lc debug, /lc mods...)"));
        coreBox_->setChecked(st.value("coreMod", true).toBool());
        form->addRow(QString(), coreBox_);
        form->addRow(QString(), creative_);
        form->addRow(QString(), damage_);

        seedEdit_ = new QLineEdit(st.value("seed").toString());
        seedEdit_->setPlaceholderText(tr("vide = aléatoire"));
        form->addRow(tr("Graine de la carte"), seedEdit_);
        root->addWidget(box);

        // ------------------------------------------------ mods supplémentaires (ZIP)
        auto *extraBox = new QGroupBox(tr("Mods supplémentaires du serveur de test (cochés = chargés au lancement)"));
        auto *ev = new QVBoxLayout(extraBox);
        extraList_ = new QListWidget;
        extraList_->setMaximumHeight(110);
        ev->addWidget(extraList_);
        auto *eb = new QHBoxLayout;
        auto *bAddZip = new QPushButton(tr("Ajouter depuis un ZIP..."));
        auto *bAddDir = new QPushButton(tr("Ajouter un dossier..."));
        auto *bDelExtra = new QPushButton(tr("Supprimer le mod sélectionné"));
        UiHooks::bind(bDelExtra, "btn.del", "server");
        eb->addWidget(bAddZip);
        eb->addWidget(bAddDir);
        eb->addWidget(bDelExtra);
        eb->addStretch();
        ev->addLayout(eb);
        root->addWidget(extraBox);
        connect(bAddZip, &QPushButton::clicked, this, [this] { addExtraFromZip(); });
        connect(bAddDir, &QPushButton::clicked, this, [this] { addExtraFromDir(); });
        connect(bDelExtra, &QPushButton::clicked, this, [this] { removeExtra(); });
        connect(extraList_, &QListWidget::itemChanged, this, [this](QListWidgetItem *) {
            if (!refreshingExtras_) saveExtraStates();
        });
        refreshExtras();

        // valeurs initiales des listes
        QString exe = st.value("exe").toString();
        if (exe.isEmpty()) exe = detectExe();
        if (!exe.isEmpty()) exeBox_->addItem(exe);
        exeBox_->setCurrentText(exe);
        QStringList games = findGames();
        for (const QString &g : {QString("devtest"), QString("minetest")})
            if (!games.contains(g)) games << g;
        gameBox_->addItems(games);
        QString defGame = st.value("game").toString();
        if (defGame.isEmpty()) defGame = games.contains("minetest") ? "minetest" : "devtest";
        gameBox_->setCurrentText(defGame);

        // ---------------------------------------------------------------- boutons
        auto *btns = new QHBoxLayout;
        auto *bStart = new QPushButton(tr("Lancer le serveur"));
        auto *bRestart = new QPushButton(tr("Relancer avec les modifications"));
        auto *bStop = new QPushButton(tr("Arrêter"));
        auto *bJoin = new QPushButton(tr("Rejoindre le serveur"));
        auto *bReset = new QPushButton(tr("Réinitialiser le monde"));
        auto *bOpen = new QPushButton(tr("Dossier du monde"));
        UiHooks::bind(bStart, "btn.start", "server");
        UiHooks::bind(bRestart, "btn.restart", "server");
        UiHooks::bind(bStop, "btn.stop", "server");
        UiHooks::bind(bJoin, "btn.join", "server");
        UiHooks::bind(bReset, "btn.reset", "server");
        UiHooks::bind(bOpen, "btn.openworld", "server");
        UiHooks::bind(bAddZip, "btn.zip", "server");
        UiHooks::bind(bAddDir, "btn.folder", "server");
        for (QPushButton *b : {bStart, bRestart, bStop, bJoin, bReset, bOpen}) {
            b->setMinimumHeight(34);
            btns->addWidget(b);
        }
        root->addLayout(btns);

        statusLbl_ = new QLabel;
        root->addWidget(statusLbl_);

        auto *hint = new QLabel(tr("À chaque lancement, le dossier local du mod (dans les données de LuantiCreator) est mis à "
                                   "jour puis copié dans le dossier « worldmods » du monde de test : les mods de votre Luanti "
                                   "ne sont jamais modifiés. En jeu, utilisez l'inventaire créatif ou "
                                   "/giveme nom_du_mod:nom_de_l_objet. Après une modification, « Relancer » puis rejoignez : "
                                   "les nouvelles textures et modèles sont envoyés à la connexion."));
        hint->setWordWrap(true);
        hint->setProperty("muted", true);
        root->addWidget(hint);

        log_ = new QPlainTextEdit;
        log_->setReadOnly(true);
        log_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        log_->setMaximumBlockCount(5000);
        auto *clear = new QPushButton(tr("Effacer la console"));
        UiHooks::bind(clear, "btn.clearlog", "server");
        root->addWidget(log_, 1);
        root->addWidget(clear, 0, Qt::AlignRight);

        // --------------------------------------------------------------- processus
        server_ = new QProcess(this);
        server_->setProcessChannelMode(QProcess::MergedChannels);
        client_ = new QProcess(this);
        client_->setProcessChannelMode(QProcess::MergedChannels);

        connect(server_, &QProcess::readyRead, this, [this] { consume(server_, sbuf_, "serveur"); });
        connect(client_, &QProcess::readyRead, this, [this] { consume(client_, cbuf_, "client"); });
        connect(server_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
            if (e == QProcess::FailedToStart) {
                log(tr("Impossible de lancer le serveur : vérifiez l'exécutable Luanti."), "#ff8a80");
                setState(Stopped);
            }
        });
        connect(client_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError e) {
            if (e == QProcess::FailedToStart) log(tr("Impossible de lancer le client Luanti."), "#ff8a80");
        });
        connect(server_, &QProcess::finished, this, [this](int code, QProcess::ExitStatus es) {
            setState(Stopped);
            flush(server_, sbuf_, "serveur");
            log(tr("■ Serveur arrêté (code %1)").arg(code), "#bbbbbb");
            if (!stopRequested_ && (code != 0 || es == QProcess::CrashExit))
                log(tr("Le serveur s'est arrêté seul : cherchez les lignes ERROR ci-dessus (erreur Lua du mod, "
                       "dépendance ou jeu manquant...)."), "#ff8a80");
            pendingJoin_ = false;
            if (restart_) {
                restart_ = false;
                startServer(rejoin_);
            }
        });
        connect(client_, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
            flush(client_, cbuf_, "client");
            log(tr("■ Client fermé (code %1)").arg(code), "#bbbbbb");
            if (restart_ && server_->state() != QProcess::NotRunning) stopServer();
            else if (restart_) {
                restart_ = false;
                startServer(rejoin_);
            }
        });

        connect(browse, &QPushButton::clicked, this, [this] {
            const QString p = QFileDialog::getOpenFileName(this, tr("Exécutable Luanti"), "/usr/games");
            if (!p.isEmpty()) exeBox_->setCurrentText(p);
        });
        connect(detect, &QPushButton::clicked, this, [this] {
            const QString p = detectExe();
            if (p.isEmpty()) log(tr("Aucun exécutable Luanti trouvé dans le PATH (luanti, minetest...)."), "#ffd180");
            else {
                exeBox_->setCurrentText(p);
                log(tr("Luanti détecté : %1").arg(p), "#a5d6a7");
            }
        });
        connect(bStart, &QPushButton::clicked, this, [this] { startServer(); });
        connect(bRestart, &QPushButton::clicked, this, [this] { restartServer(); });
        connect(bStop, &QPushButton::clicked, this, [this] { stopServer(); });
        connect(bJoin, &QPushButton::clicked, this, [this] { joinGame(); });
        connect(bReset, &QPushButton::clicked, this, [this] { resetWorld(); });
        connect(bOpen, &QPushButton::clicked, this, [this] {
            QDir().mkpath(worldDir());
            QDesktopServices::openUrl(QUrl::fromLocalFile(worldDir()));
        });
        connect(clear, &QPushButton::clicked, this, [this] { log_->clear(); });

        setState(Stopped);
        if (exeBox_->currentText().isEmpty())
            log(tr("Luanti n'a pas été trouvé automatiquement. Installez-le (sudo apt install luanti) ou "
                   "indiquez son exécutable ci-dessus."), "#ffd180");
    }

    ~ServerPage() override
    {
        restart_ = false;
        shutdown();
    }

    State state() const { return state_; }

    void startServer(bool thenJoin = false)
    {
        if (server_->state() != QProcess::NotRunning) {
            if (thenJoin) joinGame();
            return;
        }
        QString exe = exeBox_->currentText().trimmed();
        if (exe.isEmpty()) {
            exe = detectExe();
            exeBox_->setCurrentText(exe);
        }
        if (exe.isEmpty()) {
            QMessageBox::warning(this, tr("Luanti introuvable"),
                                 tr("Impossible de trouver Luanti.\n\nInstallez-le (par exemple « sudo apt install luanti ») "
                                    "ou indiquez le chemin de l'exécutable (luanti, minetest ou luantiserver)."));
            return;
        }
        const QString game = gameBox_->currentText().trimmed();
        if (game.isEmpty()) {
            QMessageBox::warning(this, tr("Jeu manquant"), tr("Indiquez le jeu à utiliser (ex : devtest ou minetest)."));
            return;
        }
        const QStringList deps = modDeps ? modDeps() : QStringList();
        if (game == "devtest" && deps.contains("default")) {
            const auto a = QMessageBox::question(
                this, tr("Dépendance « default »"),
                tr("Le jeu « devtest » ne contient pas le mod « default », or votre mod en dépend : le serveur "
                   "refusera de démarrer.\n\nChoisissez le jeu « minetest » (Minetest Game) ou retirez la dépendance "
                   "dans la page Projet.\n\nLancer quand même ?"));
            if (a != QMessageBox::Yes) return;
        }
        if (!prepare()) return;
        saveSettings();

        QStringList parts = QProcess::splitCommand(exe);
        if (parts.isEmpty()) return;
        const QString program = parts.takeFirst();
        QStringList args = parts;
        if (!isServerBinary(program)) args << "--server";
        args << "--world" << worldDir() << "--gameid" << game << "--port" << QString::number(portSpin_->value())
             << "--config" << confPath();

        log("" + program + " " + args.join(' '), "#8ab4f8");
        pendingJoin_ = thenJoin;
        stopRequested_ = false;
        sbuf_.clear();
        setState(Starting);
        server_->start(program, args);
    }

    void stopServer()
    {
        if (server_->state() == QProcess::NotRunning) return;
        stopRequested_ = true;
        log(tr("Arrêt du serveur..."), "#bbbbbb");
        server_->terminate();
        QTimer::singleShot(4000, this, [this] {
            if (server_->state() != QProcess::NotRunning) server_->kill();
        });
    }

    // exporte à nouveau le mod puis relance (le client doit se reconnecter pour recevoir les nouveautés)
    void restartServer()
    {
        const bool clientRunning = client_->state() != QProcess::NotRunning;
        if (server_->state() == QProcess::NotRunning && !clientRunning) {
            startServer();
            return;
        }
        restart_ = true;
        rejoin_ = clientRunning;
        if (clientRunning) {
            log(tr("Fermeture du client pour appliquer les modifications..."), "#bbbbbb");
            client_->terminate();
        } else {
            stopServer();
        }
    }

    void joinGame()
    {
        if (server_->state() == QProcess::NotRunning) {
            startServer(true);
            return;
        }
        if (state_ != Online) {
            pendingJoin_ = true;
            log(tr("Le client rejoindra dès que le serveur sera prêt..."), "#bbbbbb");
            return;
        }
        if (client_->state() != QProcess::NotRunning) {
            log(tr("Le client Luanti est déjà lancé."), "#ffd180");
            return;
        }
        QString exe = exeBox_->currentText().trimmed();
        QStringList parts = QProcess::splitCommand(exe);
        if (parts.isEmpty() || isServerBinary(parts.first())) {
            const QString c = detectClientExe();
            if (c.isEmpty()) {
                QMessageBox::warning(this, tr("Client introuvable"),
                                     tr("L'exécutable choisi est un serveur seul. Installez le client Luanti "
                                        "(luanti ou minetest) pour rejoindre la partie."));
                return;
            }
            parts = QStringList{c};
        }
        const QString program = parts.takeFirst();
        QStringList args = parts;
        args << "--go" << "--address" << "127.0.0.1" << "--port" << QString::number(portSpin_->value())
             << "--name" << nameEdit_->text().trimmed();
        log("" + program + " " + args.join(' '), "#8ab4f8");
        cbuf_.clear();
        client_->start(program, args);
    }

    void shutdown()
    {
        for (QProcess *p : {client_, server_}) {
            if (!p || p->state() == QProcess::NotRunning) continue;
            p->terminate();
            if (!p->waitForFinished(2500)) {
                p->kill();
                p->waitForFinished(1000);
            }
        }
    }

private:
    // ---------------------------------------------------------------- détection
    static QString detectExe()
    {
        const QStringList extra = {"/usr/games", "/usr/local/games", "/usr/local/bin", "/opt/luanti/bin", "/snap/bin"};
        for (const QString &n : {QString("luanti"), QString("minetest"), QString("luantiserver"), QString("minetestserver")}) {
            QString p = QStandardPaths::findExecutable(n);
            if (p.isEmpty()) p = QStandardPaths::findExecutable(n, extra);
            if (!p.isEmpty()) return p;
        }
        return QString();
    }

    static QString detectClientExe()
    {
        const QStringList extra = {"/usr/games", "/usr/local/games", "/usr/local/bin", "/opt/luanti/bin", "/snap/bin"};
        for (const QString &n : {QString("luanti"), QString("minetest")}) {
            QString p = QStandardPaths::findExecutable(n);
            if (p.isEmpty()) p = QStandardPaths::findExecutable(n, extra);
            if (!p.isEmpty()) return p;
        }
        return QString();
    }

    static QStringList findGames()
    {
        const QString h = QDir::homePath();
        const QStringList dirs = {"/usr/share/luanti/games", "/usr/share/minetest/games",
                                  "/usr/local/share/luanti/games", "/usr/local/share/minetest/games",
                                  h + "/.luanti/games", h + "/.minetest/games", h + "/.local/share/luanti/games"};
        QStringList out;
        for (const QString &d : dirs)
            for (const QString &g : QDir(d).entryList(QDir::Dirs | QDir::NoDotAndDotDot))
                if (!out.contains(g)) out << g;
        return out;
    }

    static bool isServerBinary(const QString &program)
    {
        return QFileInfo(program).fileName().contains("server", Qt::CaseInsensitive);
    }

    // ------------------------------------------------------------------ monde
    QString dataDir() const
    {
        QString d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
        if (d.isEmpty()) d = QDir::homePath() + "/.local/share/LuantiCreator";
        return d;
    }
    QString worldName() const
    {
        QString n = worldEdit_->text().trimmed();
        n.replace(QRegularExpression("[^A-Za-z0-9_\\-]"), "_");
        return n.isEmpty() ? QStringLiteral("creator_test") : n;
    }
    QString worldDir() const { return dataDir() + "/worlds/" + worldName(); }
    QString confPath() const { return dataDir() + "/server.conf"; }

    bool prepare()
    {
        const QString mid = modId ? modId() : QString();
        if (mid.isEmpty()) {
            QMessageBox::warning(this, tr("Mod sans identifiant"), tr("Donnez d'abord un identifiant au mod (page Projet)."));
            return false;
        }
        const QString world = worldDir();
        QDir().mkpath(world + "/worldmods");

        // 1) on met à jour le dossier local du mod (celui de LuantiCreator)
        QString modDir;
        QStringList warns;
        QString err;
        if (!buildMod || !buildMod(modDir, warns, err)) {
            log(tr("Impossible de préparer le mod : %1").arg(err), "#ff8a80");
            return false;
        }
        for (const QString &w : warns) log("Attention : " + w, "#ffd180");

        // 2) le serveur de test charge une copie de ce dossier dans le monde (worldmods)
        const QString target = world + "/worldmods/" + mid;
        QDir(target).removeRecursively();
        QString cerr;
        if (!copyDir(modDir, target, &cerr)) {
            log(tr("Impossible de charger le mod dans le monde : %1").arg(cerr), "#ff8a80");
            return false;
        }
        log(tr("Mod chargé depuis son dossier local : %1").arg(modDir), "#a5d6a7");

        // 3) mods supplémentaires de la bibliothèque (installés depuis un ZIP ou un dossier)
        for (const QString &n : extraNames())
            if (n != mid) QDir(world + "/worldmods/" + n).removeRecursively(); // nettoyage (y compris les décochés)
        QStringList loadExtras;
        for (const QString &n : enabledExtras()) {
            if (n == mid) {
                log(tr("Attention : le mod supplémentaire « %1 » porte le même nom que votre mod : ignoré.").arg(n), "#ffd180");
                continue;
            }
            QString xerr;
            if (!copyDir(libDir() + "/" + n, world + "/worldmods/" + n, &xerr)) {
                log(tr("Attention : impossible de charger « %1 » : %2").arg(n, xerr), "#ffd180");
                continue;
            }
            loadExtras << n;
        }
        if (!loadExtras.isEmpty()) log(tr("Mods supplémentaires chargés : %1").arg(loadExtras.join(", ")), "#a5d6a7");

        // 4) mod « Luanti Creator Core » (outils de test), installé automatiquement dans le monde
        const QString coreName = lccore::modName();
        bool loadCore = false;
        QDir(world + "/worldmods/" + coreName).removeRecursively();
        if (coreBox_->isChecked() && mid != coreName) {
            QString kerr;
            if (lccore::install(world + "/worldmods/" + coreName, &kerr)) {
                loadCore = true;
                log(tr("Mod « Luanti Creator Core » installé dans le monde (tapez /lc en jeu)."), "#a5d6a7");
            } else {
                log(tr("Attention : Luanti Creator Core non installé : %1").arg(kerr), "#ffd180");
            }
        }

        const QString creative = creative_->isChecked() ? "true" : "false";
        const QString damage = damage_->isChecked() ? "true" : "false";
        {
            QFile f(world + "/world.mt");
            if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                log(tr("Impossible d'écrire world.mt"), "#ff8a80");
                return false;
            }
            QTextStream s(&f);
            s << "gameid = " << gameBox_->currentText().trimmed() << "\n";
            s << "world_name = " << worldName() << "\n";
            s << "backend = sqlite3\nplayer_backend = sqlite3\nauth_backend = sqlite3\nmod_storage_backend = sqlite3\n";
            s << "creative_mode = " << creative << "\n";
            s << "enable_damage = " << damage << "\n";
            s << "load_mod_" << mid << " = true\n";
            if (loadCore) s << "load_mod_" << coreName << " = true\n";
            for (const QString &n : loadExtras) s << "load_mod_" << n << " = true\n";
        }
        {
            QFile f(confPath());
            if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                log(tr("Impossible d'écrire la configuration du serveur"), "#ff8a80");
                return false;
            }
            QTextStream s(&f);
            s << "name = " << nameEdit_->text().trimmed() << "\n";
            s << "server_announce = false\n";
            s << "motd = Serveur de test LuantiCreator\n";
            s << "creative_mode = " << creative << "\n";
            s << "enable_damage = " << damage << "\n";
            if (!seedEdit_->text().trimmed().isEmpty()) s << "fixed_map_seed = " << seedEdit_->text().trimmed() << "\n";
        }
        return true;
    }

    // ------------------------------------------------- mods supplémentaires (bibliothèque)
    // Stockés dans ~/.local/share/LuantiCreator/server_mods/<nom>/ : ils survivent à la
    // réinitialisation du monde et sont copiés dans le monde à chaque lancement.
    QString libDir() const { return dataDir() + "/server_mods"; }

    QStringList extraNames() const
    {
        return QDir(libDir()).entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name);
    }
    QStringList disabledExtras() const
    {
        return QSettings("LuantiCreator", "LuantiCreator").value("disabledExtras").toStringList();
    }
    QStringList enabledExtras() const
    {
        QStringList on;
        const QStringList off = disabledExtras();
        for (const QString &n : extraNames())
            if (!off.contains(n)) on << n;
        return on;
    }

    void refreshExtras()
    {
        refreshingExtras_ = true;
        extraList_->clear();
        const QStringList off = disabledExtras();
        for (const QString &n : extraNames()) {
            auto *it = new QListWidgetItem(n);
            it->setFlags(it->flags() | Qt::ItemIsUserCheckable);
            it->setCheckState(off.contains(n) ? Qt::Unchecked : Qt::Checked);
            const lczip::ModMeta m = lczip::parseMeta(readTextFile(libDir() + "/" + n + "/mod.conf"),
                                                      readTextFile(libDir() + "/" + n + "/depends.txt"));
            QString tip = n;
            if (!m.title.empty()) tip += " — " + QString::fromStdString(m.title);
            if (!m.depends.empty()) {
                QStringList d;
                for (const std::string &x : m.depends) d << QString::fromStdString(x);
                tip += "\n" + tr("Dépend de : %1").arg(d.join(", "));
            }
            it->setToolTip(tip);
            extraList_->addItem(it);
        }
        refreshingExtras_ = false;
    }

    void saveExtraStates()
    {
        QStringList off;
        for (int i = 0; i < extraList_->count(); ++i)
            if (extraList_->item(i)->checkState() != Qt::Checked) off << extraList_->item(i)->text();
        QSettings("LuantiCreator", "LuantiCreator").setValue("disabledExtras", off);
    }

    // Vérifie les conflits de noms puis demande confirmation avant de remplacer un mod déjà présent.
    bool acceptExtra(const ZipModInfo &m)
    {
        if (modId && m.name == modId()) {
            QMessageBox::warning(this, tr("Nom déjà utilisé"),
                                 tr("Le mod « %1 » porte le même identifiant que votre mod en cours de création. "
                                    "Renommez votre mod (page Projet) avant de l'ajouter.").arg(m.name));
            return false;
        }
        if (QDir(libDir() + "/" + m.name).exists())
            return QMessageBox::question(this, tr("Mod déjà présent"),
                                         tr("Le mod « %1 » est déjà installé sur le serveur de test. Le remplacer ?").arg(m.name))
                   == QMessageBox::Yes;
        return true;
    }

    void finishExtraInstall(const QVector<ZipModInfo> &added)
    {
        QStringList off = disabledExtras();
        QStringList names, needs;
        const QStringList have = extraNames();
        for (const ZipModInfo &m : added) {
            off.removeAll(m.name); // un mod fraîchement ajouté est activé
            names << m.name;
            for (const QString &d : m.depends)
                if (!have.contains(d) && !needs.contains(d) && !(modId && d == modId())) needs << d;
        }
        QSettings("LuantiCreator", "LuantiCreator").setValue("disabledExtras", off);
        refreshExtras();
        log(tr("Mod(s) ajouté(s) au serveur de test : %1").arg(names.join(", ")), "#a5d6a7");
        if (!needs.isEmpty())
            log(tr("Dépendances à fournir par le jeu choisi ou par un autre mod : %1").arg(needs.join(", ")), "#ffd180");
        if (server_->state() != QProcess::NotRunning) {
            if (QMessageBox::question(this, tr("Serveur en cours"),
                                      tr("Relancer le serveur maintenant pour charger le mod ?")) == QMessageBox::Yes)
                restartServer();
            else
                log(tr("Le mod sera chargé au prochain lancement du serveur."), "#bbbbbb");
        } else {
            log(tr("Le mod sera chargé au prochain lancement du serveur."), "#bbbbbb");
        }
    }

    void addExtraFromZip()
    {
        const QString zip = QFileDialog::getOpenFileName(this, tr("Mod à ajouter au serveur de test"),
                                                         QDir::homePath(), tr("Archives zip (*.zip)"));
        if (zip.isEmpty()) return;
        QVector<ZipModInfo> mods;
        QString err;
        if (!zipListMods(zip, mods, &err)) {
            QMessageBox::warning(this, tr("Archive invalide"), err);
            return;
        }
        QVector<ZipModInfo> chosen;
        for (const ZipModInfo &m : mods)
            if (acceptExtra(m)) chosen.append(m);
        if (chosen.isEmpty()) return;
        QDir().mkpath(libDir());
        if (!zipExtractMods(zip, chosen, libDir(), &err)) {
            QMessageBox::warning(this, tr("Extraction impossible"), err);
            refreshExtras();
            return;
        }
        finishExtraInstall(chosen);
    }

    void addExtraFromDir()
    {
        const QString dir = QFileDialog::getExistingDirectory(this, tr("Dossier du mod à ajouter"), QDir::homePath());
        if (dir.isEmpty()) return;
        ZipModInfo info;
        QString err;
        if (!inspectModDir(dir, info, &err)) {
            QMessageBox::warning(this, tr("Dossier invalide"), err);
            return;
        }
        if (!acceptExtra(info)) return;
        QDir().mkpath(libDir());
        QDir(libDir() + "/" + info.name).removeRecursively();
        if (!copyDir(dir, libDir() + "/" + info.name, &err)) {
            QMessageBox::warning(this, tr("Copie impossible"), err);
            refreshExtras();
            return;
        }
        finishExtraInstall(QVector<ZipModInfo>{info});
    }

    void removeExtra()
    {
        auto *it = extraList_->currentItem();
        if (!it) {
            QMessageBox::information(this, tr("Supprimer"), tr("Sélectionnez d'abord un mod dans la liste."));
            return;
        }
        const QString n = it->text();
        if (QMessageBox::question(this, tr("Supprimer le mod"),
                                  tr("Retirer « %1 » du serveur de test ?").arg(n)) != QMessageBox::Yes)
            return;
        QDir(libDir() + "/" + n).removeRecursively();
        if (server_->state() == QProcess::NotRunning) QDir(worldDir() + "/worldmods/" + n).removeRecursively();
        saveExtraStates();
        refreshExtras();
        log(tr("Mod « %1 » retiré du serveur de test.").arg(n), "#bbbbbb");
    }

    void resetWorld()
    {
        if (QMessageBox::question(this, tr("Réinitialiser le monde"),
                                  tr("Supprimer le monde de test « %1 » (carte et joueurs) ?").arg(worldName())) != QMessageBox::Yes)
            return;
        if (server_->state() != QProcess::NotRunning) {
            QMessageBox::information(this, tr("Serveur en cours"), tr("Arrêtez d'abord le serveur."));
            return;
        }
        QDir(worldDir()).removeRecursively();
        log(tr("Monde de test supprimé."), "#bbbbbb");
    }

    void saveSettings()
    {
        QSettings st("LuantiCreator", "LuantiCreator");
        st.setValue("exe", exeBox_->currentText().trimmed());
        st.setValue("game", gameBox_->currentText().trimmed());
        st.setValue("world", worldEdit_->text().trimmed());
        st.setValue("port", portSpin_->value());
        st.setValue("player", nameEdit_->text().trimmed());
        st.setValue("creative", creative_->isChecked());
        st.setValue("coreMod", coreBox_->isChecked());
        st.setValue("damage", damage_->isChecked());
        st.setValue("seed", seedEdit_->text().trimmed());
    }

    // ------------------------------------------------------------------- logs
    void log(const QString &line, const QString &color = "#d0d4da")
    {
        log_->appendHtml(QString("<span style=\"color:%1\">%2</span>").arg(color, line.toHtmlEscaped()));
    }

    void consume(QProcess *p, QByteArray &buf, const QString &tag)
    {
        buf += p->readAll();
        int i;
        while ((i = buf.indexOf('\n')) >= 0) {
            const QString line = QString::fromUtf8(buf.left(i)).trimmed();
            buf.remove(0, i + 1);
            if (!line.isEmpty()) handleLine(tag, line);
        }
    }

    void flush(QProcess *p, QByteArray &buf, const QString &tag)
    {
        consume(p, buf, tag);
        const QString rest = QString::fromUtf8(buf).trimmed();
        buf.clear();
        if (!rest.isEmpty()) handleLine(tag, rest);
    }

    void handleLine(const QString &tag, const QString &line)
    {
        QString color = "#d0d4da";
        if (line.contains("ERROR")) color = "#ff8a80";
        else if (line.contains("WARNING")) color = "#ffd180";
        else if (line.contains("listening on", Qt::CaseInsensitive)) color = "#a5d6a7";
        log("[" + tag + "] " + line, color);
        if (tag == "serveur" && state_ == Starting && line.contains("listening on", Qt::CaseInsensitive))
            setState(Online);
    }

    void setState(State s)
    {
        state_ = s;
        QString txt, col;
        switch (s) {
        case Stopped: txt = tr("Serveur arrêté"); col = "#9e9e9e"; break;
        case Starting: txt = tr("Démarrage du serveur..."); col = "#ffb300"; break;
        case Online:
            txt = tr("Serveur en ligne sur 127.0.0.1:%1").arg(portSpin_->value());
            col = "#43a047";
            break;
        }
        statusLbl_->setText(QString("<span style=\"color:%1;font-weight:bold;\">● %2</span>").arg(col, txt.toHtmlEscaped()));
        if (onStatus) onStatus(txt, int(s));
        if (s == Online && pendingJoin_) {
            pendingJoin_ = false;
            joinGame();
        }
    }

    QComboBox *exeBox_, *gameBox_;
    QLineEdit *worldEdit_, *nameEdit_, *seedEdit_;
    QSpinBox *portSpin_;
    QCheckBox *creative_, *damage_, *coreBox_;
    QLabel *statusLbl_;
    QListWidget *extraList_ = nullptr;
    bool refreshingExtras_ = false;
    QPlainTextEdit *log_;
    QProcess *server_ = nullptr, *client_ = nullptr;
    QByteArray sbuf_, cbuf_;
    State state_ = Stopped;
    bool pendingJoin_ = false, stopRequested_ = false, restart_ = false, rejoin_ = false;
};

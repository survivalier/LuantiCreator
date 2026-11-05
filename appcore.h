#pragma once
// Éléments partagés entre les fenêtres de LuantiCreator (lanceur, éditeur de mod, éditeur de plugin) :
//  - emplacements des projets et des dossiers de mods ;
//  - AppBus : petit canal de notification (« les plugins ont changé ») entre fenêtres ;
//  - AppFlow : ouverture du lanceur / de l'éditeur d'un mod (branché dans main()).

#include <QtWidgets>
#include <algorithm>
#include <functional>

#include "codehl.h"
#include "winframe.h"

inline QString lcAppDir()
{
    QString d = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    if (d.isEmpty()) d = QDir::homePath() + "/.local/share/LuantiCreator";
    return d;
}
inline QString lcModsWorkspace() { return lcAppDir() + "/mods"; }
inline QString lcProjectsDir() { return lcAppDir() + "/projects"; }
inline QString lcImportsDir() { return lcAppDir() + "/imports"; }
inline QString lcProjectFile(const QString &id) { return lcProjectsDir() + "/" + id + ".lcproj"; }

// ---- Thème : une seule table de couleurs (palette Qt + feuille de style). Un plugin autorisé (luanti_creator_core) peut
//      modifier ces couleurs, la police, et ajouter ses propres feuilles de style ; tout est réappliqué par lcApplyTheme().
using LcColors = QMap<QString, QString>;

inline LcColors lcBaseColors(bool dark)
{
    if (dark)
        return {{"bg", "#1e2024"}, {"surface", "#272a30"}, {"surface2", "#2f3238"}, {"border", "#3a3e46"}, {"text", "#e4e6ea"},
                {"muted", "#9096a2"}, {"accent", "#5b8def"}, {"accentHover", "#76a0f5"}, {"accentText", "#ffffff"},
                {"hover", "#33373e"}, {"scroll", "#454a53"}, {"danger", "#ef6b6b"}, {"disabled", "#6b7280"}, {"placeholder", "#9096a2"}};
    return {{"bg", "#f4f5f7"}, {"surface", "#ffffff"}, {"surface2", "#eceef2"}, {"border", "#d9dce3"}, {"text", "#22252b"},
            {"muted", "#6b7280"}, {"accent", "#3b6fd4"}, {"accentHover", "#2f5cb8"}, {"accentText", "#ffffff"},
            {"hover", "#e6e9ef"}, {"scroll", "#c5c9d2"}, {"danger", "#d03a3a"}, {"disabled", "#a0a5b0"}, {"placeholder", "#9aa0ad"}};
}

// Réglages venant des plugins (remplis par main.cpp à partir de PluginHost::uiPatches()).
struct LcStyleOverrides {
    LcColors colors;       // noms de lcBaseColors() -> "#rrggbb"
    QString fontFamily;    // vide : police du système
    double fontSize = 0;   // 0 : taille par défaut
    QString qss;           // feuilles de style supplémentaires, appliquées après celle de LuantiCreator
};
inline LcStyleOverrides &lcOverrides()
{
    static LcStyleOverrides o;
    return o;
}
inline bool lcIsColorName(const QString &n) { return lcBaseColors(true).contains(n); }
inline bool lcIsHexColor(const QString &v)
{
    static const QRegularExpression re("^#[0-9a-fA-F]{6}$");
    return re.match(v).hasMatch();
}

inline QString lcBuildStyleSheet(const LcColors &c)
{
    QString q = QString::fromUtf8(R"QSS(
* { outline: none; }
QMainWindow, QDialog, QWidget#centralwidget { background: @bg; }
QToolTip { background: @surface2; color: @text; border: 1px solid @border; border-radius: 6px; padding: 5px 8px; }

QMenuBar { background: @bg; color: @text; padding: 3px 6px; border-bottom: 1px solid @border; }
QMenuBar::item { background: transparent; padding: 5px 11px; border-radius: 6px; }
QMenuBar::item:selected { background: @hover; }
QMenu { background: @surface; color: @text; border: 1px solid @border; border-radius: 10px; padding: 6px; }
QMenu::item { padding: 6px 28px 6px 14px; border-radius: 6px; }
QMenu::item:selected { background: @accent; color: @accentText; }
QMenu::separator { height: 1px; background: @border; margin: 5px 8px; }

QPushButton, QToolButton { background: @surface2; color: @text; border: 1px solid @border; border-radius: 8px; padding: 6px 14px; }
QPushButton:hover, QToolButton:hover { background: @hover; border-color: @accent; }
QPushButton:pressed, QToolButton:pressed { background: @border; }
QPushButton:default { background: @accent; color: @accentText; border-color: @accent; }
QPushButton:default:hover { background: @accentHover; }
QPushButton:disabled, QToolButton:disabled { background: transparent; color: @muted; border-color: @border; }
QToolButton::menu-indicator { subcontrol-position: right center; right: 6px; }
QPushButton#card { background: @surface; border: 1px solid @border; border-radius: 10px; padding: 10px; }
QPushButton#card:hover { background: @surface2; border: 1px solid @accent; }
QPushButton#card:pressed { background: @hover; }

QLineEdit, QTextEdit, QPlainTextEdit, QSpinBox, QDoubleSpinBox, QComboBox {
    background: @surface; color: @text; border: 1px solid @border; border-radius: 8px; padding: 5px 9px;
    selection-background-color: @accent; selection-color: @accentText; }
QLineEdit:hover, QSpinBox:hover, QDoubleSpinBox:hover, QComboBox:hover { border-color: @muted; }
QLineEdit:focus, QTextEdit:focus, QPlainTextEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus { border: 1px solid @accent; }
QLineEdit:disabled, QSpinBox:disabled, QDoubleSpinBox:disabled, QComboBox:disabled { color: @muted; background: transparent; }
QComboBox { padding-right: 24px; }
QComboBox::drop-down { border: none; width: 22px; }
QComboBox QAbstractItemView { background: @surface; color: @text; border: 1px solid @border; border-radius: 8px;
    selection-background-color: @accent; selection-color: @accentText; padding: 4px; outline: none; }

QCheckBox, QRadioButton { spacing: 8px; color: @text; }
QCheckBox:disabled, QRadioButton:disabled { color: @muted; }
QGroupBox { background: transparent; border: 1px solid @border; border-radius: 8px; margin-top: 14px; padding: 14px 10px 10px 10px; }
QGroupBox::title { subcontrol-origin: margin; subcontrol-position: top left; left: 14px; padding: 0 6px; color: @muted; font-weight: bold; }

QTabWidget::pane { border: 1px solid @border; border-radius: 8px; background: @surface; top: -1px; }
QTabBar { background: transparent; }
QTabBar::tab { background: transparent; color: @muted; padding: 8px 18px; margin: 0 2px; border: none; border-bottom: 2px solid transparent; }
QTabBar::tab:hover { color: @text; }
QTabBar::tab:selected { color: @accent; border-bottom: 2px solid @accent; font-weight: bold; }

QListView, QListWidget, QTreeView, QTreeWidget, QTableView, QTableWidget {
    background: @surface; color: @text; border: 1px solid @border; border-radius: 8px; padding: 3px;
    alternate-background-color: @surface2; selection-background-color: @accent; selection-color: @accentText; }
QListView::item, QListWidget::item, QTreeView::item { padding: 5px 6px; border-radius: 6px; }
QListView::item:hover, QListWidget::item:hover, QTreeView::item:hover { background: @hover; }
QListView::item:selected, QListWidget::item:selected, QTreeView::item:selected { background: @accent; color: @accentText; }
QHeaderView::section { background: @surface2; color: @muted; border: none; border-bottom: 1px solid @border; padding: 6px 8px; font-weight: bold; }
QTableView { gridline-color: @border; }

QScrollArea { border: none; background: transparent; }
QScrollBar:vertical { background: transparent; width: 12px; margin: 2px; }
QScrollBar:horizontal { background: transparent; height: 12px; margin: 2px; }
QScrollBar::handle:vertical { background: @scroll; border-radius: 4px; min-height: 30px; margin: 0 2px; }
QScrollBar::handle:horizontal { background: @scroll; border-radius: 4px; min-width: 30px; margin: 2px 0; }
QScrollBar::handle:hover { background: @muted; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QScrollBar::add-page, QScrollBar::sub-page { background: transparent; }

QSplitter::handle { background: transparent; }
QSplitter::handle:hover { background: @accent; }
QProgressBar { background: @surface2; border: none; border-radius: 6px; text-align: center; color: @text; min-height: 12px; }
QProgressBar::chunk { background: @accent; border-radius: 6px; }
QSlider::groove:horizontal { height: 5px; background: @border; border-radius: 2px; }
QSlider::sub-page:horizontal { background: @accent; border-radius: 2px; }
QSlider::handle:horizontal { background: @accent; width: 14px; height: 14px; margin: -5px 0; border-radius: 7px; }
QStatusBar { background: @bg; color: @muted; border-top: 1px solid @border; }
QToolBar { background: @bg; border: none; spacing: 4px; padding: 4px; }
QDockWidget { color: @text; }
QDockWidget::title { background: @surface2; padding: 6px; border-radius: 6px; }
QPushButton[primary="true"] { background: @accent; color: @accentText; border-color: @accent; font-weight: bold; }
QPushButton[primary="true"]:hover { background: @accentHover; border-color: @accentHover; }
QPushButton[primary="true"]:disabled { background: transparent; color: @muted; border-color: @border; }
QLabel[muted="true"] { color: @muted; }
QLabel[role="title"] { font-size: 26px; font-weight: 600; }
QLabel[role="heading"] { font-size: 18px; font-weight: 600; }
QLabel[role="warning"] { color: #d9972b; font-weight: 600; }
QFrame[frameShape="4"], QFrame[frameShape="5"] { color: @border; }
)QSS");
    // remplace les @jetons, les plus longs d'abord (@accentHover avant @accent...)
    QStringList keys = c.keys();
    std::sort(keys.begin(), keys.end(), [](const QString &a, const QString &b) { return a.size() > b.size(); });
    for (const QString &k : keys) q.replace("@" + k, c.value(k));
    return q;
}

inline QString &lcBaseQss()
{
    static QString q;
    return q;
}

// Réapplique la feuille de style globale (thème + feuilles des plugins)
inline void lcRefreshStyleSheet()
{
    qApp->setStyleSheet(lcBaseQss() + "\n" + lcOverrides().qss);
}

inline void lcApplyTheme(bool dark)
{
    QApplication::setStyle("Fusion");
    LcColors c = lcBaseColors(dark);
    for (auto it = lcOverrides().colors.begin(); it != lcOverrides().colors.end(); ++it)
        if (c.contains(it.key()) && lcIsHexColor(it.value())) c[it.key()] = it.value();

    static const QFont systemFont = qApp->font(); // police d'origine, pour pouvoir annuler un changement de plugin
    QFont f = systemFont;
    if (f.pointSizeF() > 0 && f.pointSizeF() < 10) f.setPointSizeF(10);
    if (!lcOverrides().fontFamily.isEmpty()) f.setFamily(lcOverrides().fontFamily);
    if (lcOverrides().fontSize >= 6 && lcOverrides().fontSize <= 32) f.setPointSizeF(lcOverrides().fontSize);
    qApp->setFont(f);

    QPalette p;
    auto set = [&](QPalette::ColorRole r, const char *key) { p.setColor(r, QColor(c.value(key))); };
    set(QPalette::Window, "bg");           set(QPalette::WindowText, "text");
    set(QPalette::Base, "surface");        set(QPalette::AlternateBase, "surface2");
    set(QPalette::ToolTipBase, "surface2"); set(QPalette::ToolTipText, "text");
    set(QPalette::Text, "text");           set(QPalette::Button, "surface2");
    set(QPalette::ButtonText, "text");     set(QPalette::BrightText, "danger");
    set(QPalette::Link, "accent");         set(QPalette::Highlight, "accent");
    set(QPalette::HighlightedText, "accentText"); set(QPalette::PlaceholderText, "placeholder");
    set(QPalette::Mid, "border");
    for (auto r : {QPalette::Text, QPalette::ButtonText, QPalette::WindowText})
        p.setColor(QPalette::Disabled, r, QColor(c.value("disabled")));
    qApp->setPalette(p);
    lcBaseQss() = lcBuildStyleSheet(c);
    lcRefreshStyleSheet();
    LcHighlighter::rehighlightAll(); // la coloration du code suit le thème (vues déjà ouvertes comprises)
}

// Notifications entre fenêtres. Un abonné est lié à un QObject (désabonnement automatique quand il disparaît) ;
// `except` : l'émetteur ne reçoit pas sa propre notification.
struct AppBus {
    struct Sub {
        QPointer<QObject> owner;
        QString topic;
        std::function<void()> fn;
    };
    static inline QList<Sub> &subs()
    {
        static QList<Sub> l;
        return l;
    }
    static void subscribe(QObject *owner, const QString &topic, std::function<void()> fn)
    {
        Sub s;
        s.owner = owner;
        s.topic = topic;
        s.fn = std::move(fn);
        subs().append(s);
    }
    static void notify(QObject *except, const QString &topic)
    {
        const QList<Sub> copy = subs(); // un abonné peut s'abonner ou disparaître pendant l'appel
        for (const Sub &s : copy)
            if (s.owner && s.owner.data() != except && s.topic == topic) s.fn();
        auto &l = subs();
        for (int i = l.size() - 1; i >= 0; --i)
            if (!l[i].owner) l.removeAt(i);
    }
};

// Navigation entre les fenêtres principales (installé par main()).
struct AppFlow {
    static inline std::function<void()> showLauncher;                    // ouvre (ou ramène) le lanceur
    static inline std::function<void(const QString &)> openProject;      // ouvre l'éditeur du projet (.lcproj)
    static inline QPointer<QWidget> lastOpened;                          // dernière fenêtre ouverte par showLauncher / openProject
    static inline std::function<void()> reloadApp;                       // recharge toute l'application (plugins, style, fenêtres)
};

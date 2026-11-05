// LuantiCreator - créateur de mods visuel pour Luanti (ex-Minetest)
// Qt6 Widgets / C++17

#include <QtWidgets>
#ifndef LC_VERSION
#define LC_VERSION "0.7"
#endif
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <memory>
#include <vector>

#include "appcore.h"
#include "codehl.h"
#include "editor.h"
#include "generator.h"
#include "launcher.h"
#include "mesh.h"
#include "formspec.h"
#include "fsutil.h"
#include "pluginpage.h"
#include "plugins.h"
#include "preview.h"
#include "uihooks.h"
#include "server.h"
#include "splash.h"
#include "widgets.h"

static const QStringList kModes = {
    "Boîte englobante (AABB)", "Boîte réduite", "Voxels (multi-boîtes précises)",
    "Cube complet", "Manuelle"};

static const char *kModelFilter = "Modèles 3D (*.obj *.gltf *.glb *.b3d *.x);;Tous les fichiers (*)";

struct RefEntry {
    QString ref, label, category;
    bool plugin = false; // fourni par un plugin
};

// Blocs et objets courants de Minetest Game (catégories sans accents : leurs icônes sont « cat.<nom> »).
static const QVector<RefEntry> &defaultEntries()
{
    static QVector<RefEntry> v;
    if (!v.isEmpty()) return v;
    struct D { const char *cat, *ref, *label; };
    static const D tbl[] = {
        {"Terrain", "default:dirt", "Terre"}, {"Terrain", "default:dirt_with_grass", "Terre herbeuse"},
        {"Terrain", "default:dirt_with_dry_grass", "Terre à herbe sèche"}, {"Terrain", "default:dirt_with_snow", "Terre enneigée"},
        {"Terrain", "default:dirt_with_rainforest_litter", "Terre de jungle"}, {"Terrain", "default:dry_dirt", "Terre sèche"},
        {"Terrain", "default:dry_dirt_with_dry_grass", "Terre sèche herbeuse"}, {"Terrain", "default:permafrost", "Pergélisol"},
        {"Terrain", "default:sand", "Sable"}, {"Terrain", "default:desert_sand", "Sable du désert"},
        {"Terrain", "default:silver_sand", "Sable argenté"}, {"Terrain", "default:gravel", "Gravier"},
        {"Terrain", "default:clay", "Argile"}, {"Terrain", "default:snowblock", "Bloc de neige"},
        {"Terrain", "default:snow", "Neige"}, {"Terrain", "default:ice", "Glace"},
        {"Terrain", "default:cave_ice", "Glace de grotte"}, {"Terrain", "default:water_source", "Eau"},
        {"Terrain", "default:river_water_source", "Eau de rivière"}, {"Terrain", "default:lava_source", "Lave"},
        {"Pierre", "default:stone", "Pierre"}, {"Pierre", "default:cobble", "Pierre taillée"},
        {"Pierre", "default:mossycobble", "Pierre moussue"}, {"Pierre", "default:stonebrick", "Briques de pierre"},
        {"Pierre", "default:stone_block", "Bloc de pierre"}, {"Pierre", "default:desert_stone", "Pierre du désert"},
        {"Pierre", "default:desert_cobble", "Pierre taillée du désert"}, {"Pierre", "default:desert_stonebrick", "Briques du désert"},
        {"Pierre", "default:sandstone", "Grès"}, {"Pierre", "default:sandstonebrick", "Briques de grès"},
        {"Pierre", "default:desert_sandstone", "Grès du désert"}, {"Pierre", "default:silver_sandstone", "Grès argenté"},
        {"Pierre", "default:obsidian", "Obsidienne"}, {"Pierre", "default:obsidianbrick", "Briques d'obsidienne"},
        {"Pierre", "default:brick", "Briques"},
        {"Bois", "default:tree", "Tronc de chêne"}, {"Bois", "default:wood", "Planches de chêne"},
        {"Bois", "default:leaves", "Feuilles de chêne"}, {"Bois", "default:sapling", "Pousse de chêne"},
        {"Bois", "default:pine_tree", "Tronc de pin"}, {"Bois", "default:pine_wood", "Planches de pin"},
        {"Bois", "default:pine_needles", "Aiguilles de pin"}, {"Bois", "default:jungletree", "Tronc de jungle"},
        {"Bois", "default:junglewood", "Planches de jungle"}, {"Bois", "default:jungleleaves", "Feuilles de jungle"},
        {"Bois", "default:acacia_tree", "Tronc d'acacia"}, {"Bois", "default:acacia_wood", "Planches d'acacia"},
        {"Bois", "default:acacia_leaves", "Feuilles d'acacia"}, {"Bois", "default:aspen_tree", "Tronc de tremble"},
        {"Bois", "default:aspen_wood", "Planches de tremble"}, {"Bois", "default:aspen_leaves", "Feuilles de tremble"},
        {"Bois", "default:bush_leaves", "Feuilles de buisson"}, {"Bois", "default:cactus", "Cactus"},
        {"Bois", "default:papyrus", "Papyrus"}, {"Bois", "default:dry_shrub", "Arbuste sec"},
        {"Bois", "default:grass_1", "Herbe"}, {"Bois", "default:junglegrass", "Herbe de jungle"},
        {"Minerais", "default:stone_with_coal", "Minerai de charbon"}, {"Minerais", "default:stone_with_iron", "Minerai de fer"},
        {"Minerais", "default:stone_with_copper", "Minerai de cuivre"}, {"Minerais", "default:stone_with_tin", "Minerai d'étain"},
        {"Minerais", "default:stone_with_gold", "Minerai d'or"}, {"Minerais", "default:stone_with_mese", "Minerai de mese"},
        {"Minerais", "default:stone_with_diamond", "Minerai de diamant"}, {"Minerais", "default:mese", "Bloc de mese"},
        {"Minerais", "default:steelblock", "Bloc d'acier"}, {"Minerais", "default:copperblock", "Bloc de cuivre"},
        {"Minerais", "default:bronzeblock", "Bloc de bronze"}, {"Minerais", "default:goldblock", "Bloc d'or"},
        {"Minerais", "default:diamondblock", "Bloc de diamant"}, {"Minerais", "default:tinblock", "Bloc d'étain"},
        {"Verre", "default:glass", "Verre"}, {"Verre", "default:obsidian_glass", "Verre d'obsidienne"},
        {"Verre", "default:meselamp", "Lampe de mese"}, {"Verre", "default:torch", "Torche"},
        {"Objets", "default:stick", "Bâton"}, {"Objets", "default:coal_lump", "Charbon"},
        {"Objets", "default:iron_lump", "Minerai de fer (objet)"}, {"Objets", "default:copper_lump", "Minerai de cuivre (objet)"},
        {"Objets", "default:tin_lump", "Minerai d'étain (objet)"}, {"Objets", "default:gold_lump", "Minerai d'or (objet)"},
        {"Objets", "default:steel_ingot", "Lingot d'acier"}, {"Objets", "default:copper_ingot", "Lingot de cuivre"},
        {"Objets", "default:tin_ingot", "Lingot d'étain"}, {"Objets", "default:bronze_ingot", "Lingot de bronze"},
        {"Objets", "default:gold_ingot", "Lingot d'or"}, {"Objets", "default:diamond", "Diamant"},
        {"Objets", "default:mese_crystal", "Cristal de mese"}, {"Objets", "default:paper", "Papier"},
        {"Objets", "default:book", "Livre"}, {"Objets", "default:flint", "Silex"},
        {"Nourriture", "default:apple", "Pomme"}, {"Nourriture", "farming:bread", "Pain"},
        {"Nourriture", "farming:wheat", "Blé"}, {"Nourriture", "farming:flour", "Farine"},
    };
    for (const D &d : tbl) v.append({QString::fromUtf8(d.ref), QString::fromUtf8(d.label), QString::fromUtf8(d.cat), false});
    return v;
}

static void applyTheme(bool dark) { lcApplyTheme(dark); }

// ------------------------------------------------------------ page Blocs 3D

class ModelsPage : public QWidget {
public:
    EditorTab *tab;

    ModelsPage(QWidget *parent = nullptr) : QWidget(parent)
    {
        QVector<Field> f = {
            {"name", tr("Nom technique"), TText, {}, ""},
            {"description", tr("Nom affiché"), TText, {}, "Bloc 3D"},
            {"file", tr("Modèle 3D (.obj conseillé)"), TFile, {}, "", 9999, 0, kModelFilter,
             tr(".obj : aperçu et selectionbox complète. .gltf/.glb : boîte englobante seulement.")},
            {"scale", tr("Unités du modèle par node"), TDouble, {}, "1", 1000, 0, "",
             tr("Taille du modèle divisée par cette valeur pour obtenir des nodes. "
                "À ajuster si le modèle paraît trop grand ou trop petit en jeu.")},
            {"tex1", tr("Texture 1"), TFile, {}, ""},
            {"tex2", tr("Texture 2 (optionnel)"), TFile, {}, ""},
            {"tex3", tr("Texture 3 (optionnel)"), TFile, {}, ""},
            {"tex4", tr("Texture 4 (optionnel)"), TFile, {}, ""},
            {"anim_time", tr("Animations : durée d'un cycle (secondes)"), TDouble, {}, "1", 600, 0,
             "", tr("Une texture en bande verticale (hauteur = N × largeur) est animée automatiquement.")},
            {"group", tr("Groupe de minage"), TCombo,
             {"cracky", "choppy", "crumbly", "snappy", "oddly_breakable_by_hand"}, "choppy"},
            {"level", tr("Dureté (1-3)"), TInt, {}, "2", 3},
            {"light", tr("Lumière émise (0-14)"), TInt, {}, "0", 14},
            {"rotate", tr("Orientable (facedir)"), TCheck, {}, "1"},
            {"collision", tr("Utiliser les boîtes aussi pour la collision"), TCheck, {}, "1"},
            {"selmode", tr("Mode de génération de la selectionbox"), TCombo, kModes, kModes[0]},
            {"grid", tr("Voxels : résolution (2-64)"), TInt, {}, "12", 64, 2},
            {"pad", tr("Boîte réduite : marge (nodes)"), TDouble, {}, "0.02", 1, 0},
            {"maxboxes", tr("Voxels : nombre max de boîtes"), TInt, {}, "24", 500, 1},
            {"boxes", tr("Selectionbox (une boîte par ligne : x1 y1 z1 x2 y2 z2)"), TMulti, {}, ""},
        };
        tab = new EditorTab("bloc3d_", f);
        tab->iconFn = [](const QVariantMap &m) { return iconFromImage(texOf(m, "tex1", false, false)); };

        view_ = new ModelView;
        strip_ = new PreviewPane(paintTexStrip, 84);
        strip_->setMinimumWidth(100);
        info_ = new QLabel;
        info_->setWordWrap(true);
        auto *gen = new QPushButton(tr("Générer la selectionbox automatiquement"));
        UiHooks::bind(gen, "btn.selectbox", "models");
        gen->setMinimumHeight(38);
        auto *refreshBtn = new QPushButton(tr("Actualiser l'aperçu"));
        UiHooks::bind(refreshBtn, "btn.refresh", "models");

        auto *right = new QVBoxLayout;
        right->addWidget(view_, 1);
        right->addWidget(strip_);
        right->addWidget(info_);
        right->addWidget(gen);
        right->addWidget(refreshBtn);

        auto *h = new QHBoxLayout(this);
        h->addWidget(tab, 3);
        h->addLayout(right, 2);

        tab->onChanged = [this](const QVariantMap &m) {
            strip_->setData(m);
            refresh();
        };
        connect(gen, &QPushButton::clicked, this, [this] { generate(); });
        connect(refreshBtn, &QPushButton::clicked, this, [this] {
            cache_.path.clear();
            refresh();
        });
    }

private:
    void refresh()
    {
        const QVariantMap m = tab->currentMap();
        if (m.isEmpty()) {
            view_->setMesh(Mesh(), 1);
            view_->setBoxes({});
            info_->setText(tr("Ajoutez un bloc 3D puis importez un modèle."));
            return;
        }
        const double scale = m["scale"].toDouble() > 0 ? m["scale"].toDouble() : 1.0;
        const Mesh &mesh = cache_.get(m["file"].toString());
        view_->setMesh(mesh, scale);
        view_->setBoxes(parseBoxes(m["boxes"].toString()));
        if (mesh.valid) {
            const QVector3D d = (mesh.mx - mesh.mn) / float(scale);
            info_->setText(tr("Taille : %1 × %2 × %3 nodes — %4 sommets, %5 triangles")
                               .arg(numStr(d.x()), numStr(d.y()), numStr(d.z()))
                               .arg(mesh.v.size()).arg(mesh.tris.size()));
        } else {
            info_->setText(mesh.error);
        }
    }

    void generate()
    {
        const QVariantMap m = tab->currentMap();
        if (m.isEmpty()) return;
        const int mode = kModes.indexOf(m["selmode"].toString());
        const double scale = m["scale"].toDouble() > 0 ? m["scale"].toDouble() : 1.0;
        const Mesh &mesh = cache_.get(m["file"].toString());
        if (!mesh.valid && mode != 3 && mode != 4) {
            QMessageBox::warning(this, tr("Selectionbox"),
                                 mesh.error.isEmpty() ? tr("Importez d'abord un modèle 3D.") : mesh.error);
            return;
        }
        if (mode == 4) {
            QMessageBox::information(this, tr("Selectionbox"),
                                     tr("Mode manuel : saisissez les boîtes dans le champ « Selectionbox »."));
            return;
        }
        const QVector<Box6> boxes = generateBoxes(mesh, mode, scale, m["pad"].toDouble(),
                                                  m["grid"].toInt(), m["maxboxes"].toInt());
        tab->setValue("boxes", formatBoxes(boxes));
        refresh();
        info_->setText(info_->text() + tr("\n%1 boîte(s) générée(s).").arg(boxes.size()));
    }

    ModelView *view_ = nullptr;
    PreviewPane *strip_ = nullptr;
    QLabel *info_ = nullptr;
    MeshCache cache_;
};

// ------------------------------------------------------------------ fenêtre

// Fenêtre d'ÉDITION d'un mod. Elle est ouverte par le lanceur (LauncherWindow) avec le projet choisi ;
// « Fichier → Gestionnaire de projets » la ferme et rouvre le lanceur.
// Style demandé par les plugins autorisés (luanti_creator_core : set_theme_colors, set_font, add_stylesheet...).
// Valable pour TOUTES les fenêtres : lanceur, éditeur de mod, éditeur de plugin. Se base sur PluginHost::uiPatches().
static void lcApplyPluginStyle()
{
    LcStyleOverrides o;
    for (const PluginHost::UiPatch &up : plugins().uiPatches()) {
        const QJsonObject &d = up.data;
        const QString op = d["op"].toString();
        if (op == "theme") {
            const QJsonObject c = d["colors"].toObject();
            for (auto it = c.begin(); it != c.end(); ++it) o.colors[it.key()] = it.value().toString();
        } else if (op == "font") {
            if (d.contains("family")) o.fontFamily = d["family"].toString();
            if (d.contains("size")) o.fontSize = d["size"].toDouble();
        } else if (op == "stylesheet") {
            o.qss += d["qss"].toString() + "\n";
        }
    }
    lcOverrides() = o;
    QSettings st("LuantiCreator", "LuantiCreator");
    lcApplyTheme(st.value("dark", true).toBool());
}

class MainWindow : public QMainWindow {
public:
    explicit MainWindow(const QString &projectPath = QString())
    {
        setAttribute(Qt::WA_DeleteOnClose);
        active_ = this;
        setWindowTitle("LuantiCreator");
        setProperty("lcHelpTopic", "L'éditeur de mod"); // section de la documentation ouverte par F1 / « ? »
        LcChrome::install(this); // cadre personnalisé (style du cadre de chargement)
        resize(1320, 840);
        restoreGeometry(QSettings("LuantiCreator", "LuantiCreator").value("editorGeometry").toByteArray()); // taille et place mémorisées

        stack_ = new QStackedWidget;
        setCentralWidget(stack_);
        home_ = new QWidget;
        stack_->addWidget(home_);

        // --- projet
        auto *proj = new QWidget;
        auto *pf = new QFormLayout(proj);
        modName_ = new QLineEdit("mon_mod");
        modTitle_ = new QLineEdit("Mon mod");
        modDesc_ = new QLineEdit;
        modDeps_ = new QLineEdit("default");
        modDeps_->setPlaceholderText("ex : default, farming");
        pf->addRow(tr("Identifiant du mod (a-z, 0-9, _)"), modName_);
        pf->addRow(tr("Titre"), modTitle_);
        pf->addRow(tr("Description"), modDesc_);
        pf->addRow(tr("Dépendances (séparées par des virgules)"), modDeps_);
        auto *projNote = new QLabel(tr("Astuce : gardez « default » en dépendance si vos biomes, minerais ou recettes "
                                       "utilisent des blocs de Minetest Game (default:stone...). Dans ce cas, lancez le "
                                       "serveur de test avec le jeu « minetest »."));
        projNote->setWordWrap(true);
        pf->addRow(projNote);
        modPathLbl_ = new QLabel;
        modPathLbl_->setWordWrap(true);
        modPathLbl_->setTextInteractionFlags(Qt::TextSelectableByMouse);
        auto *openFolderBtn = new QPushButton(tr("Ouvrir le dossier du mod"));
        UiHooks::bind(openFolderBtn, "btn.folder", "project");
        pf->addRow(tr("Dossier local du mod (chargé par le serveur de test)"), modPathLbl_);
        pf->addRow(QString(), openFolderBtn);
        connect(openFolderBtn, &QPushButton::clicked, this, [this] { openModFolder(); });
        auto *autoNote = new QLabel(tr("Le projet est enregistré automatiquement et le dossier du mod est régénéré à chaque "
                                       "modification. Ne modifiez pas ce dossier à la main : il est recréé à chaque mise à jour."));
        autoNote->setWordWrap(true);
        autoNote->setProperty("muted", true);
        pf->addRow(autoNote);

        // --- blocs
        nodes_ = new EditorTab("bloc_", {
            {"name", tr("Nom technique"), TText, {}, ""},
            {"description", tr("Nom affiché"), TText, {}, "Nouveau bloc"},
            {"texture", tr("Texture (PNG)"), TFile, {}, ""},
            {"overlay", tr("Texture par-dessus (optionnel)"), TFile, {}, "", 9999, 0, "Images (*.png)",
             tr("Posée sur la texture de base (transparence respectée). Les deux sont fusionnées en un seul PNG dans le mod.")},
            {"overlay2", tr("2e texture par-dessus (optionnel)"), TFile, {}, "", 9999, 0, "Images (*.png)"},
            {"overlay_opacity", tr("Opacité des textures par-dessus (%)"), TInt, {}, "100", 100, 0},
            {"anim", tr("Animation"), TCombo, lctx::modeLabels(), lctx::modeLabels()[0], 9999, 0, "",
             tr("Une texture animée est une image verticale dont chaque image est un carré (hauteur = N × largeur). « Auto » la détecte toute seule.")},
            {"anim_x", tr("Animation : nombre d'images (bande) / colonnes (planche)"), TInt, {}, "1", 1024, 1},
            {"anim_y", tr("Animation : lignes (planche)"), TInt, {}, "1", 1024, 1},
            {"anim_time", tr("Animation : durée d'un cycle complet (secondes)"), TDouble, {}, "1", 600, 0},
            {"drawtype", tr("Forme (drawtype)"), TCombo,
             {"normal", "glasslike", "allfaces", "plantlike", "torchlike", "airlike"}, "normal"},
            {"group", tr("Groupe de minage"), TCombo,
             {"cracky", "choppy", "crumbly", "snappy", "oddly_breakable_by_hand"}, "cracky"},
            {"level", tr("Dureté (1-3)"), TInt, {}, "2", 3},
            {"light", tr("Lumière émise (0-14)"), TInt, {}, "0", 14},
            {"walkable", tr("Solide (on peut marcher dessus)"), TCheck, {}, "1"},
            {"stack", tr("Taille de pile max"), TInt, {}, "99", 999},
            {"drop", tr("Objet lâché (vide = le bloc lui-même)"), TRef, {}, ""},
        });
        nodes_->iconFn = pathIcon("texture");

        models_ = new ModelsPage;

        items_ = new EditorTab("objet_", {
            {"name", tr("Nom technique"), TText, {}, ""},
            {"description", tr("Nom affiché"), TText, {}, "Nouvel objet"},
            {"texture", tr("Texture (PNG)"), TFile, {}, ""},
            {"overlay", tr("Texture par-dessus (optionnel)"), TFile, {}, "", 9999, 0, "Images (*.png)",
             tr("Posée sur la texture de base (transparence respectée). Les deux sont fusionnées en un seul PNG dans le mod.")},
            {"overlay2", tr("2e texture par-dessus (optionnel)"), TFile, {}, "", 9999, 0, "Images (*.png)"},
            {"overlay_opacity", tr("Opacité des textures par-dessus (%)"), TInt, {}, "100", 100, 0},
            {"stack", tr("Taille de pile max"), TInt, {}, "99", 999},
        });
        items_->iconFn = pathIcon("texture");

        foods_ = new EditorTab("nourriture_", {
            {"name", tr("Nom technique"), TText, {}, ""},
            {"description", tr("Nom affiché"), TText, {}, "Nouvelle nourriture"},
            {"texture", tr("Texture (PNG)"), TFile, {}, ""},
            {"overlay", tr("Texture par-dessus (optionnel)"), TFile, {}, "", 9999, 0, "Images (*.png)",
             tr("Posée sur la texture de base (transparence respectée). Les deux sont fusionnées en un seul PNG dans le mod.")},
            {"overlay2", tr("2e texture par-dessus (optionnel)"), TFile, {}, "", 9999, 0, "Images (*.png)"},
            {"overlay_opacity", tr("Opacité des textures par-dessus (%)"), TInt, {}, "100", 100, 0},
            {"hp", tr("Points de vie rendus (demi-cœurs)"), TInt, {}, "4", 20},
            {"stack", tr("Taille de pile max"), TInt, {}, "99", 999},
        });
        foods_->iconFn = pathIcon("texture");

        tools_ = new EditorTab("outil_", {
            {"name", tr("Nom technique"), TText, {}, ""},
            {"description", tr("Nom affiché"), TText, {}, "Nouvel outil"},
            {"texture", tr("Texture (PNG)"), TFile, {}, ""},
            {"overlay", tr("Texture par-dessus (optionnel)"), TFile, {}, "", 9999, 0, "Images (*.png)",
             tr("Posée sur la texture de base (transparence respectée). Les deux sont fusionnées en un seul PNG dans le mod.")},
            {"overlay2", tr("2e texture par-dessus (optionnel)"), TFile, {}, "", 9999, 0, "Images (*.png)"},
            {"overlay_opacity", tr("Opacité des textures par-dessus (%)"), TInt, {}, "100", 100, 0},
            {"group", tr("Groupe miné"), TCombo, {"cracky", "choppy", "crumbly", "snappy"}, "cracky"},
            {"level", tr("Niveau max (1-3)"), TInt, {}, "2", 3},
            {"uses", tr("Durabilité (utilisations)"), TInt, {}, "30"},
            {"t1", tr("Temps de minage niveau 1 (s × 10)"), TInt, {}, "40"},
            {"t2", tr("Temps de minage niveau 2 (s × 10)"), TInt, {}, "20"},
            {"t3", tr("Temps de minage niveau 3 (s × 10)"), TInt, {}, "10"},
        });
        tools_->iconFn = pathIcon("texture");

        QVector<Field> cf = {
            {"name", tr("Nom de la recette"), TText, {}, ""},
            {"kind", tr("Type"), TCombo, {"shaped", "shapeless", "cooking", "fuel"}, "shaped"},
            {"output", tr("Résultat"), TRef, {}, ""},
            {"count", tr("Quantité produite"), TInt, {}, "1", 99},
            {"time", tr("Cuisson / combustible : durée (s)"), TInt, {}, "10", 9999},
        };
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                cf.push_back(Field{QString("r%1%2").arg(r).arg(c),
                                   tr("Ingrédient : ligne %1, colonne %2").arg(r + 1).arg(c + 1), TRef, {}, "",
                                   9999, 0, "",
                                   (r == 0 && c == 0) ? tr("Pour cuisson/combustible : seul cet ingrédient est utilisé.")
                                                      : QString()});
        crafts_ = new EditorTab("recette_", cf);
        crafts_->iconFn = refIcon("output");

        ores_ = new EditorTab("minerai_", {
            {"name", tr("Nom"), TText, {}, ""},
            {"type", tr("Type"), TCombo, {"scatter", "blob"}, "scatter"},
            {"ore", tr("Bloc du minerai"), TRef, {}, ""},
            {"wherein", tr("Généré dans"), TRef, {}, "default:stone"},
            {"scarcity", tr("Rareté (1 minerai tous les N nodes³)"), TInt, {}, "512", 100000, 1},
            {"num", tr("Minerais par veine"), TInt, {}, "8", 64, 1},
            {"size", tr("Taille de la veine"), TInt, {}, "3", 32, 1},
            {"ymin", tr("Altitude min"), TInt, {}, "-256", 31000, -31000},
            {"ymax", tr("Altitude max"), TInt, {}, "64", 31000, -31000},
        });
        ores_->iconFn = refIcon("ore");

        biomes_ = new EditorTab("biome_", {
            {"name", tr("Nom du biome"), TText, {}, ""},
            {"node_top", tr("Bloc de surface"), TRef, {}, "default:dirt_with_grass"},
            {"depth_top", tr("Épaisseur de surface"), TInt, {}, "1", 64},
            {"node_filler", tr("Bloc de remplissage"), TRef, {}, "default:dirt"},
            {"depth_filler", tr("Épaisseur de remplissage"), TInt, {}, "3", 64},
            {"node_stone", tr("Bloc de roche"), TRef, {}, "default:stone"},
            {"ymax", tr("Altitude max"), TInt, {}, "200", 31000, -31000},
            {"ymin", tr("Altitude min"), TInt, {}, "-10", 31000, -31000},
            {"heat", tr("Température (0-100)"), TInt, {}, "50", 100},
            {"humid", tr("Humidité (0-100)"), TInt, {}, "50", 100},
            {"replace", tr("Remplacer les biomes et décorations existants"), TCheck, {}, "0", 9999, 0, "",
             tr("Appelle clear_registered_biomes() : seuls vos biomes existeront.")},
        });
        biomes_->iconFn = refIcon("node_top");

        decos_ = new EditorTab("deco_", {
            {"name", tr("Nom"), TText, {}, ""},
            {"deco", tr("Bloc placé"), TRef, {}, ""},
            {"place_on", tr("Posé sur (liste séparée par des virgules)"), TText, {}, "default:dirt_with_grass"},
            {"biomes", tr("Biomes (liste, vide = tous)"), TText, {}, ""},
            {"fill", tr("Densité (pour 1000)"), TInt, {}, "20", 1000},
            {"ymin", tr("Altitude min"), TInt, {}, "1", 31000, -31000},
            {"ymax", tr("Altitude max"), TInt, {}, "200", 31000, -31000},
        });
        decos_->iconFn = refIcon("deco");

        entities_ = new EditorTab("entite_", {
            {"name", tr("Nom technique"), TText, {}, ""},
            {"visual", tr("Apparence"), TCombo, {"mesh", "cube", "sprite", "upright_sprite"}, "mesh"},
            {"file", tr("Modèle 3D (si apparence = mesh)"), TFile, {}, "", 9999, 0, kModelFilter},
            {"scale", tr("Unités du modèle par node"), TDouble, {}, "1", 1000, 0},
            {"texture", tr("Texture (PNG)"), TFile, {}, ""},
            {"auto_cbox", tr("Collisionbox automatique d'après le modèle"), TCheck, {}, "1"},
            {"cbox", tr("Collisionbox manuelle (x1 y1 z1 x2 y2 z2)"), TText, {}, "-0.3 -0.5 -0.3 0.3 0.5 0.3"},
            {"hp", tr("Points de vie"), TInt, {}, "10", 1000, 1},
            {"behaviour", tr("Comportement"), TCombo, {"immobile", "wander", "follow"}, "wander"},
            {"speed", tr("Vitesse"), TDouble, {}, "1.5", 20, 0},
        });
        entities_->iconFn = pathIcon("texture");

        cmds_ = new EditorTab("commande_", {
            {"name", tr("Nom de la commande"), TText, {}, ""},
            {"params", tr("Paramètres (aide)"), TText, {}, ""},
            {"description", tr("Description"), TText, {}, ""},
            {"priv", tr("Privilège requis (vide = aucun)"), TText, {}, ""},
            {"message", tr("Message renvoyé ({player} = nom du joueur)"), TText, {}, "Bonjour {player} !"},
        });

        t_ = Tabs{nodes_, models_->tab, items_, foods_, tools_, crafts_,
                  ores_, biomes_, decos_, entities_, cmds_};
        pctx().tabs = &t_;
        pctx().modid = identOf(modName_->text());
        pctx().pluginTexture = [](const QString &r) { return plugins().texture(r); };
        auto reg = [this](const char *k, EditorTab *t) { lists_.emplace_back(QString(k), t); };
        reg("nodes", nodes_); reg("models", models_->tab); reg("items", items_); reg("foods", foods_);
        reg("tools", tools_); reg("crafts", crafts_); reg("ores", ores_); reg("biomes", biomes_);
        reg("decos", decos_); reg("entities", entities_); reg("cmds", cmds_);

        // --- aperçus en direct
        panes_.push_back(attachPreview(nodes_, paintNode, 300));
        panes_.push_back(attachPreview(items_, itemPainter(0)));
        panes_.push_back(attachPreview(foods_, itemPainter(1)));
        panes_.push_back(attachPreview(tools_, itemPainter(2)));
        panes_.push_back(attachPreview(crafts_, paintCraft, 260));
        panes_.push_back(attachPreview(ores_, paintOre, 280));
        panes_.push_back(attachPreview(biomes_, paintBiome, 520));
        panes_.push_back(attachPreview(decos_, paintDeco, 260));
        panes_.push_back(attachPreview(cmds_, paintCommand));

        {   // entités : aperçu 3D + fiche
            auto *ev = new ModelView;
            auto *ep = new PreviewPane(paintEntity, 170);
            auto cache = std::make_shared<MeshCache>();
            auto *w = new QWidget;
            auto *v = new QVBoxLayout(w);
            v->setContentsMargins(0, 0, 0, 0);
            v->addWidget(ev, 3);
            v->addWidget(ep, 2);
            entities_->setSide(w);
            entities_->onChanged = [ev, ep, cache](const QVariantMap &m) {
                ep->setData(m);
                if (m.isEmpty() || m["visual"].toString() != "mesh" || m["file"].toString().isEmpty()) {
                    ev->setMesh(Mesh(), 1);
                    ev->setBoxes({});
                    return;
                }
                const double sc = m["scale"].toDouble() > 0 ? m["scale"].toDouble() : 1.0;
                const Mesh &mesh = cache->get(m["file"].toString());
                ev->setMesh(mesh, sc);
                QVector<Box6> boxes;
                if (m["auto_cbox"].toBool() && mesh.valid) boxes = generateBoxes(mesh, 0, sc, 0, 0, 0);
                else boxes = parseBoxes(m["cbox"].toString());
                ev->setBoxes(boxes);
            };
            panes_.push_back(ep);
        }

        // listes de références (blocs du mod + blocs des plugins + Minetest Game), sélecteur et modèles prédéfinis
        EditorHooks::refList = [this] {
            // listes déroulantes : au plus 300 blocs de plugins (le bouton « … » ouvre la liste complète)
            QStringList l;
            QSet<QString> seen;
            int plug = 0;
            for (const RefEntry &e : refEntries()) {
                if (seen.contains(e.ref)) continue;
                if (e.plugin && ++plug > 300) continue;
                seen.insert(e.ref);
                l << e.ref;
            }
            return l;
        };
        EditorHooks::refIcon = [this](const QString &ref) {
            auto it = iconCache_.find(ref);
            if (it != iconCache_.end()) return it.value();
            const QIcon ic = iconFromImage(imageForRef(ref), 20);
            iconCache_.insert(ref, ic);
            return ic;
        };
        EditorHooks::pickRef = [this](QWidget *parent, const QString &cur) { return pickBlock(parent, cur); };
        EditorHooks::presets = [](const QString &tab) {
            QVector<QPair<QString, QVariantMap>> r;
            for (const PluginPreset &p : plugins().presets(tab)) r.append(qMakePair(p.name, p.values));
            return r;
        };
        for (const auto &l : lists_) l.second->setTabKey(l.first);
        connect(modName_, &QLineEdit::textChanged, this, [this](const QString &t) {
            pctx().modid = identOf(t);
            refreshAll();
        });

        // --- code Lua
        auto *prevPage = new QWidget;
        auto *pv = new QVBoxLayout(prevPage);
        preview_ = new QPlainTextEdit;
        preview_->setReadOnly(true);
        preview_->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        lcAttachHighlighter(preview_, LcLang::Lua);
        auto *prevRefresh = new QPushButton(tr("Actualiser"));
        UiHooks::bind(prevRefresh, "btn.refresh", "preview");
        pv->addWidget(prevRefresh);
        pv->addWidget(preview_);
        connect(prevRefresh, &QPushButton::clicked, this, [this] { refreshPreview(); });

        // --- serveur de test
        server_ = new ServerPage;
        server_->modId = [this] { return identOf(modName_->text()); };
        server_->modDeps = [this] { return allDeps(nullptr); };
        server_->buildMod = [this](QString &dir, QStringList &warns, QString &err) {
            if (!sync(true, &warns, &err)) return false;
            dir = modFolder();
            return true;
        };

        // --- accueil
        auto *homeLayout = new QVBoxLayout(home_);
        auto *title = new QLabel("LuantiCreator");
        title->setProperty("role", "title");
        title->setAlignment(Qt::AlignCenter);
        auto *sub = new QLabel(tr("Créez des mods Luanti sans écrire de Lua. Choisissez une fonctionnalité :"));
        sub->setAlignment(Qt::AlignCenter);
        sub->setProperty("muted", true);
        grid_ = new QGridLayout;
        grid_->setSpacing(14);
        homeLayout->addSpacing(6);
        homeLayout->addWidget(title);
        homeLayout->addWidget(sub);
        hintLbl_ = new QLabel(tr("👉 Pour commencer : ouvrez « Blocs » et ajoutez votre premier bloc, puis cliquez sur « Tester mon mod maintenant »."));
        hintLbl_->setAlignment(Qt::AlignCenter);
        hintLbl_->setWordWrap(true);
        hintLbl_->setProperty("muted", true);
        homeLayout->addWidget(hintLbl_);
        homeLayout->addSpacing(6);
        homeLayout->addLayout(grid_, 1);
        auto *testBtn = new QPushButton(tr("Tester mon mod maintenant"));
        testBtn->setMinimumHeight(48);
        testBtn->setProperty("primary", true);
        homeLayout->addWidget(testBtn);
        UiHooks::bind(testBtn, "btn.test", "home");
        connect(testBtn, &QPushButton::clicked, this, [this] { server_->joinGame(); });

        addCard("project", "", tr("Projet"), tr("Nom, titre, dépendances"), page("project", tr("Projet"), proj), nullptr, {});
        addCard("nodes", "", tr("Blocs"), tr("Blocs cubiques et formes simples"), page("nodes", tr("Blocs"), nodes_), nodes_, {});
        addCard("models", "", tr("Blocs 3D"), tr("Modèles 3D + selectionbox auto"), page("models", tr("Blocs 3D (modèles)"), models_), models_->tab, {});
        addCard("items", "", tr("Objets"), tr("Objets d'inventaire"), page("items", tr("Objets"), items_), items_, {});
        addCard("foods", "", tr("Nourriture"), tr("Objets comestibles"), page("foods", tr("Nourriture"), foods_), foods_, {});
        addCard("tools", "", tr("Outils"), tr("Pioches, haches, pelles"), page("tools", tr("Outils"), tools_), tools_, {});
        addCard("crafts", "", tr("Recettes"), tr("Craft, cuisson, combustibles"), page("crafts", tr("Recettes"), crafts_), crafts_, {});
        addCard("ores", "", tr("Minerais"), tr("Génération de minerais"), page("ores", tr("Minerais"), ores_), ores_, {});
        addCard("biomes", "", tr("Biomes"), tr("Terrains, altitude, climat"), page("biomes", tr("Biomes"), biomes_), biomes_, {});
        addCard("decos", "", tr("Décorations"), tr("Plantes, arbres, décor"), page("decos", tr("Décorations"), decos_), decos_, {});
        addCard("entities", "", tr("Entités"), tr("Créatures avec modèle 3D"), page("entities", tr("Entités"), entities_), entities_, {});
        addCard("cmds", "", tr("Commandes"), tr("Commandes de chat"), page("cmds", tr("Commandes"), cmds_), cmds_, {});
        addCard("pixel", "", tr("Éditeur de pixels"), tr("Dessinez vos textures"), page("pixel", tr("Éditeur de pixels"), new PixelEditor), nullptr, {});
        formspec_ = new FormspecDesigner;
        addCard("formspec", "", tr("Formspec"), tr("Interfaces par glisser-déposer"), page("formspec", tr("Éditeur de formspec"), formspec_), nullptr, {});
        previewPage_ = page("preview", tr("Aperçu du code Lua"), prevPage);
        addCard("lua", "", tr("Code Lua"), tr("Voir le code généré"), previewPage_, nullptr, {});
        addCard("server", "", tr("Serveur de test"), tr("Lancer et rejoindre un serveur"), page("server", tr("Serveur de test"), server_), nullptr, {});
        addCard("folder", "", tr("Dossier du mod"), tr("Ouvrir le dossier local du mod"), nullptr, nullptr, [this] { openModFolder(); });
        addCard("export", "", tr("Exporter"), tr("Installer dans Luanti ou créer un .zip"), nullptr, nullptr, [this] { exportMod(); });
        addCard("save", "", tr("Enregistrer"), tr("Forcer la mise à jour du projet"), nullptr, nullptr, [this] { saveProject(); });
        addCard("open", "", tr("Projets"), tr("Retour au gestionnaire de projets"), nullptr, nullptr, [this] { backToLauncher(); });

        pluginPage_ = new PluginPage(PluginPageMode::Project); // extraits Lua et scripts ; la gestion est dans le lanceur
        pluginPage_->onChanged = [this] { onPluginsChanged(); };
        pluginPage_->runScript = [this](const QString &pid, const QString &sid) { return runPluginScript(pid, sid); };
        addCard("plugins", "", tr("Plugins du mod"), tr("Extraits Lua et scripts des plugins"), page("plugins", tr("Plugins du mod"), pluginPage_), nullptr, {});
        rebuildFeatures();

        connect(stack_, &QStackedWidget::currentChanged, this, [this](int) {
            refreshAll();
            if (stack_->currentWidget() == previewPage_) refreshPreview();
        });

        // --- menus
        auto *fm = menuBar()->addMenu(tr("&Fichier"));
        if (QAction *homeAct = addAct(fm, tr("&Accueil du mod"), [this] { stack_->setCurrentWidget(home_); }, QKeySequence("Ctrl+H"), "menu.home"))
            homeAct->setShortcuts({QKeySequence("Ctrl+H"), QKeySequence("Alt+Left")}); // Alt+Gauche : comme « page précédente »
        addAct(fm, tr("&Gestionnaire de projets (ferme cette fenêtre)"), [this] { backToLauncher(); }, QKeySequence("Ctrl+Shift+H"), "menu.projects");
        addAct(fm, tr("&Enregistrer"), [this] { saveProject(); }, QKeySequence::Save, "menu.save");
        addAct(fm, tr("Enregistrer une &copie sous..."), [this] { saveCopy(); }, QKeySequence(), "menu.savecopy");
        fm->addSeparator();
        addAct(fm, tr("Ouvrir le &dossier du mod"), [this] { openModFolder(); }, QKeySequence(), "menu.folder");
        addAct(fm, tr("&Exporter (installer dans Luanti / .zip)..."), [this] { exportMod(); }, QKeySequence("Ctrl+E"), "menu.export");
        fm->addSeparator();
        addAct(fm, tr("&Quitter"), [this] { close(); }, QKeySequence::Quit, "menu.quit");

        auto *vm = menuBar()->addMenu(tr("&Affichage"));
        auto *darkAct = addAct(vm, tr("Thème &sombre"), [] {}, QKeySequence(), "menu.dark");
        darkAct->setCheckable(true);
        {
            QSettings st("LuantiCreator", "LuantiCreator");
            const bool dark = st.value("dark", true).toBool();
            darkAct->setChecked(dark);
            applyTheme(dark);
        }
        connect(darkAct, &QAction::toggled, this, [this](bool on) {
            QSettings st("LuantiCreator", "LuantiCreator");
            st.setValue("dark", on);
            applyTheme(on);
            for (PreviewPane *p : panes_) p->update();
        });

        auto *simpleAct = addAct(vm, tr("&Mode simple (masquer les fonctions avancées)"), [] {}, QKeySequence(), "menu.simple");
        simpleAct->setCheckable(true);
        simpleAct->setStatusTip(tr("N'affiche que l'essentiel. Les fonctions avancées restent accessibles avec Ctrl+K."));
        simpleMode_ = QSettings("LuantiCreator", "LuantiCreator").value("simpleMode", false).toBool();
        simpleAct->setChecked(simpleMode_);
        connect(simpleAct, &QAction::toggled, this, [this](bool on) {
            simpleMode_ = on;
            QSettings("LuantiCreator", "LuantiCreator").setValue("simpleMode", on);
            applyCorePatches();
            statusBar()->showMessage(on ? tr("Mode simple activé : fonctions avancées masquées (Ctrl+K pour les retrouver).")
                                        : tr("Mode simple désactivé."), 6000);
        });

        auto *pm = menuBar()->addMenu(tr("&Plugins"));
        addAct(pm, tr("&Gérer les plugins (nouvelle fenêtre)..."), [] { PluginManagerWindow::open(); }, QKeySequence(), "menu.plugins");

        auto *tm = menuBar()->addMenu(tr("&Serveur de test"));
        addAct(tm, tr("Lancer"), [this] { server_->startServer(); }, QKeySequence("F5"), "menu.start");
        addAct(tm, tr("Relancer avec les modifications"), [this] { server_->restartServer(); }, QKeySequence("Ctrl+F5"), "menu.restart");
        addAct(tm, tr("Rejoindre"), [this] { server_->joinGame(); }, QKeySequence("F6"), "menu.join");
        addAct(tm, tr("Arrêter"), [this] { server_->stopServer(); }, QKeySequence("Shift+F5"), "menu.stop");

        auto *hm = menuBar()->addMenu(tr("A&ide"));
        if (QAction *docAct = addAct(hm, tr("&Documentation de LuantiCreator"), [] { ApiDocWindow::open(); }, QKeySequence(), "menu.apidoc"))
            docAct->setStatusTip(tr("Guide complet : créer un mod, tester, exporter, plugins (F1)"));
        addAct(hm, tr("&Aller à... (chercher une page ou une action)"), [this] { openQuickSearch(); }, QKeySequence("Ctrl+K"), "menu.goto");
        addAct(hm, tr("&Premiers pas et raccourcis"), [this] {
            QMessageBox::information(this, tr("Premiers pas"),
                                     tr("<b>Pour commencer</b><br>"
                                        "1. Choisissez une carte sur l'accueil (par exemple « Blocs »).<br>"
                                        "2. Ajoutez un élément et réglez-le : l'aperçu se met à jour tout seul.<br>"
                                        "3. Cliquez sur « Tester mon mod maintenant » (F5 lance le serveur) ou exportez (Ctrl+E).<br><br>"
                                        "Tout est enregistré automatiquement.<br><br>"
                                        "<b>Raccourcis</b><br>"
                                        "Ctrl+K : aller à une page ou une action<br>"
                                        "Ctrl+H ou Alt+Gauche : accueil du mod<br>"
                                        "Ctrl+Maj+H : gestionnaire de projets<br>"
                                        "Ctrl+N : ajouter un élément — Ctrl+D : dupliquer — Suppr : supprimer — Ctrl+Z : annuler la suppression<br>"
                                        "Ctrl+E : exporter — F5 : lancer — Ctrl+F5 : relancer — F6 : rejoindre — Maj+F5 : arrêter<br><br>"
                                        "Trop de cartes ? Menu Affichage → Mode simple."));
        }, QKeySequence(), "menu.help");
        addAct(hm, tr("À propos"), [this] {
            QMessageBox::about(this, "LuantiCreator",
                               tr("LuantiCreator %1\nCréateur visuel de mods pour Luanti.\n\n"
                                  "Ctrl+K : aller à... — Ctrl+H : accueil du mod — Ctrl+Shift+H : projets — Ctrl+E : exporter — F5 : lancer le serveur de test — F6 : rejoindre")
                                   .arg(LC_VERSION));
        }, QKeySequence(), "menu.about");

        // --- barre d'outils du serveur de test
        auto *tb = addToolBar(tr("Serveur de test"));
        tb->setMovable(false);
        tb->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        QAction *actHome = addAct(tb, tr("Accueil"), [this] { stack_->setCurrentWidget(home_); }, QKeySequence());
        actHome->setToolTip(tr("Retour à l'accueil du mod (Ctrl+H)"));
        UiHooks::bind(actHome, "btn.home", "toolbar");
        QAction *actExport = addAct(tb, tr("Exporter"), [this] { exportMod(); }, QKeySequence());
        actExport->setToolTip(tr("Installer le mod dans Luanti ou créer un .zip (Ctrl+E)"));
        UiHooks::bind(actExport, "btn.export", "toolbar");
        tb->addSeparator();
        actStart_ = addAct(tb, tr("Lancer le serveur de test"), [this] { server_->startServer(); });
        actRestart_ = addAct(tb, tr("Relancer avec les modifications"), [this] { server_->restartServer(); });
        actJoin_ = addAct(tb, tr("Rejoindre"), [this] { server_->joinGame(); });
        actStop_ = addAct(tb, tr("Arrêter"), [this] { server_->stopServer(); });
        UiHooks::bind(actStart_, "btn.start", "toolbar");
        UiHooks::bind(actRestart_, "btn.restart", "toolbar");
        UiHooks::bind(actJoin_, "btn.join", "toolbar");
        UiHooks::bind(actStop_, "btn.stop", "toolbar");
        tb->addSeparator();
        statusLbl_ = new QLabel;
        tb->addWidget(statusLbl_);
        server_->onStatus = [this](const QString &txt, int st) {
            const char *col = st == 2 ? "#43a047" : st == 1 ? "#ffb300" : "#9e9e9e";
            statusLbl_->setText(QString(" <span style=\"color:%1\">●</span> %2").arg(col, txt.toHtmlEscaped()));
            actStart_->setEnabled(st == 0);
            actRestart_->setEnabled(st != 0);
            actStop_->setEnabled(st != 0);
        };
        server_->onStatus(tr("Serveur arrêté"), 0);

        statusPathLbl_ = new QLabel;
        statusBar()->addPermanentWidget(statusPathLbl_);
        coreLbl_ = new QLabel;
        coreLbl_->setProperty("role", "warning");
        coreLbl_->hide();
        statusBar()->addPermanentWidget(coreLbl_);
        QStringList startupMsgs;
        plugins().runStartup(startupMsgs); // scripts « startup » : modifications de LuantiCreator par les plugins autorisés
        UiHooks::refresh();
        refreshCardIcons();
        applyCorePatches();

        // --- projet choisi dans le lanceur (ou nouveau projet si l'ouverture échoue), puis enregistrement automatique
        if (projectPath.isEmpty() || !QFile::exists(projectPath) || !loadProjectFile(projectPath, false)) {
            modName_->setText(freshId());
            lastBuilt_.clear();
            refreshAll();
            sync(true);
        }
        updatePathLabels();
        UiHooks::bindWindow(this, "editor");
        syncTimer_ = new QTimer(this);
        syncTimer_->setInterval(2500);
        connect(syncTimer_, &QTimer::timeout, this, [this] { sync(false); });
        syncTimer_->start();
        statusBar()->showMessage(tr("Prêt. Choisissez une carte pour commencer — Ctrl+K pour chercher une page."), 10000);
        // un plugin modifié / installé / supprimé dans une autre fenêtre (gestionnaire, éditeur de plugin)
        AppBus::subscribe(this, "plugins", [this] {
            pluginPage_->rebuild();
            onPluginsChanged();
        });
        if (!startupMsgs.isEmpty()) statusBar()->showMessage(startupMsgs.join(" — "), 12000);
    }

    ~MainWindow() override
    {
        // les accroches globales pointent vers cette fenêtre : on les retire si personne ne les a reprises
        if (active_ == this) {
            EditorHooks::refList = nullptr;
            EditorHooks::refIcon = nullptr;
            EditorHooks::pickRef = nullptr;
            EditorHooks::presets = nullptr;
            pctx().tabs = nullptr;
            pctx().pluginTexture = nullptr;
            active_ = nullptr;
        }
    }

    // fichier du projet ouvert (vide si pas encore enregistré)
    QString currentProjectFile() const { return lastBuilt_.isEmpty() ? QString() : projectFile(lastBuilt_); }

protected:
    void closeEvent(QCloseEvent *e) override
    {
        QSettings("LuantiCreator", "LuantiCreator").setValue("editorGeometry", saveGeometry());
        sync(true);
        server_->shutdown();
        QMainWindow::closeEvent(e);
    }

private:
    struct Card {
        QString id, emoji, title, desc;
        EditorTab *counter;
        QLabel *sub, *iconLbl, *titleLbl;
        QPushButton *btn;
        bool dynamic;
        QString baseTitle, baseDesc, baseEmoji; // valeurs d'origine (luanti_creator_core peut les modifier)
        bool hidden = false;                    // carte masquée par un plugin autorisé
    };

    // `key` : clé d'icône de l'action (« menu.<action> », voir uihooks.h), éventuellement dans un contexte `scope`
    QAction *addAct(QWidget *w, const QString &text, std::function<void()> fn,
                    const QKeySequence &ks = QKeySequence(), const QString &key = QString(), const QString &scope = QString())
    {
        auto *a = new QAction(text, this);
        if (!ks.isEmpty()) a->setShortcut(ks);
        connect(a, &QAction::triggered, this, [fn] { fn(); });
        w->addAction(a);
        if (!key.isEmpty()) UiHooks::bind(a, key, scope);
        return a;
    }

    static std::function<QIcon(const QVariantMap &)> pathIcon(const char *key)
    {
        const QString k = key;
        return [k](const QVariantMap &m) { return iconFromImage(texOf(m, k, true, false)); };
    }
    static std::function<QIcon(const QVariantMap &)> refIcon(const char *key)
    {
        const QString k = key;
        return [k](const QVariantMap &m) { return iconFromImage(imageForRef(m[k].toString())); };
    }

    QWidget *page(const QString &id, const QString &title, QWidget *content)
    {
        auto *w = new QWidget;
        auto *v = new QVBoxLayout(w);
        auto *bar = new QHBoxLayout;
        auto *back = new QPushButton(tr("Accueil"));
        UiHooks::bind(back, "btn.back", "page." + id);
        auto *lab = new QLabel(title);
        lab->setProperty("role", "heading");
        bar->addWidget(back);
        bar->addWidget(lab, 1);
        v->addLayout(bar);
        v->addWidget(content, 1);
        stack_->addWidget(w);
        connect(back, &QPushButton::clicked, this, [this] { stack_->setCurrentWidget(home_); });
        return w;
    }

    QPushButton *addCard(const QString &id, const QString &icon, const QString &title, const QString &desc, QWidget *target,
                         EditorTab *counter, std::function<void()> action, bool dynamic = false)
    {
        auto *btn = new QPushButton;
        btn->setObjectName("card");
        btn->setMinimumSize(170, 112);
        btn->setCursor(Qt::PointingHandCursor);
        auto *v = new QVBoxLayout(btn);
        auto *i = new QLabel(icon);
        i->setStyleSheet("font-size:26px;");
        auto *t = new QLabel(title);
        t->setStyleSheet("font-size:14px;font-weight:600;");
        auto *d = new QLabel(desc);
        d->setWordWrap(true);
        d->setProperty("muted", true);
        for (QLabel *l : {i, t, d}) {
            l->setAttribute(Qt::WA_TransparentForMouseEvents);
            l->setAlignment(Qt::AlignCenter);
            v->addWidget(l);
        }
        cards_.push_back(Card{id, icon, title, desc, counter, d, i, t, btn, dynamic, title, desc, icon, false});
        applyCardIcon(cards_.back());
        if (target) connect(btn, &QPushButton::clicked, this, [this, target] { stack_->setCurrentWidget(target); });
        else connect(btn, &QPushButton::clicked, this, [action] { if (action) action(); });
        relayout();
        return btn;
    }

    // l'icône d'un plugin (« card.<id> ») s'affiche sur la carte ; sans icône, la carte n'a que son titre
    void applyCardIcon(const Card &c)
    {
        const QIcon ic = UiHooks::get("card." + c.id);
        if (!ic.isNull()) {
            c.iconLbl->setText(QString());
            c.iconLbl->setPixmap(ic.pixmap(QSize(44, 44)));
            c.iconLbl->setVisible(true);
            const bool only = UiHooks::isIconOnly("card." + c.id);
            c.titleLbl->setVisible(!only); // « icon_only » : l'icône seule, sans titre ni description
            c.sub->setVisible(!only);
            c.btn->setToolTip(only ? c.title + " — " + c.desc : QString());
        } else {
            c.titleLbl->setVisible(true);
            c.sub->setVisible(true);
            c.btn->setToolTip(QString());
            c.iconLbl->setPixmap(QPixmap());
            c.iconLbl->setText(c.emoji); // vide, sauf si un plugin autorisé a donné un « emoji » à la carte
            c.iconLbl->setVisible(!c.emoji.isEmpty());
        }
    }

    void refreshCardIcons()
    {
        for (const Card &c : cards_) applyCardIcon(c);
    }

    void relayout()
    {
        while (grid_->count()) delete grid_->takeAt(0); // retire les éléments de la grille (les boutons survivent)
        int n = 0;
        for (int pass = 0; pass < 2; ++pass)
            for (const Card &c : cards_)
                if (c.dynamic == (pass == 1)) {
                    if (c.hidden) {
                        c.btn->hide();
                        continue;
                    }
                    grid_->addWidget(c.btn, n / 5, n % 5);
                    c.btn->show();
                    ++n;
                }
    }

    void updateCards()
    {
        int total = 0;
        for (const Card &c : cards_)
            if (c.counter) {
                c.sub->setText(tr("%1 — %2 élément(s)").arg(c.desc).arg(c.counter->count()));
                total += c.counter->count();
            }
        if (hintLbl_) hintLbl_->setVisible(total == 0); // l'aide de démarrage disparaît dès que le mod contient quelque chose
    }

    // ------------------------------------------------------------ plugins
    std::vector<std::pair<QString, EditorTab *>> allTabs() const
    {
        std::vector<std::pair<QString, EditorTab *>> r = lists_;
        for (const auto &f : featTabs_) r.emplace_back(f.first, f.second);
        return r;
    }

    // (re)crée les pages et les cartes des fonctionnalités fournies par les plugins ; les données sont conservées
    void rebuildFeatures()
    {
        for (const auto &f : featTabs_) featData_[f.first] = f.second->toJson();
        if (std::find(featPages_.begin(), featPages_.end(), stack_->currentWidget()) != featPages_.end())
            stack_->setCurrentWidget(home_); // la page affichée va disparaître
        for (QWidget *w : featPages_) {
            stack_->removeWidget(w);
            delete w;
        }
        featPages_.clear();
        featTabs_.clear();
        for (int i = int(cards_.size()) - 1; i >= 0; --i)
            if (cards_[size_t(i)].dynamic) {
                cards_[size_t(i)].btn->hide();
                cards_[size_t(i)].btn->deleteLater();
                cards_.erase(cards_.begin() + i);
            }
        for (const PluginFeature &f : plugins().features()) {
            auto *tab = new EditorTab(f.prefix, f.fields);
            tab->setTabKey(f.key);
            tab->fromJson(featData_.value(f.key));
            featTabs_.emplace_back(f.key, tab);
            QWidget *pg = page(f.key, f.title, tab);
            featPages_.push_back(pg);
            addCard(f.key, "", f.title, f.desc.isEmpty() ? tr("Fonctionnalité d'un plugin") : f.desc, pg, tab, {}, true);
        }
        relayout();
    }

    // ------------------------------------------------------------ luanti_creator_core
    // Remet LuantiCreator dans son état d'origine puis applique les modifications des plugins autorisés
    // (titre, style, cartes). Les icônes passent par PluginHost::icon(). Un avertissement permanent s'affiche.
    void applyCorePatches()
    {
        setWindowTitle("LuantiCreator");
        for (Card &c : cards_) {
            c.title = c.baseTitle;
            c.desc = c.baseDesc;
            c.emoji = c.baseEmoji;
            c.hidden = false;
            c.titleLbl->setText(c.title);
            c.sub->setText(c.desc);
        }
        for (const PluginHost::UiPatch &up : plugins().uiPatches()) {
            const QJsonObject &d = up.data;
            const QString op = d["op"].toString();
            if (op == "window_title") {
                setWindowTitle(d["text"].toString());
            } else if (op == "card") {
                for (Card &c : cards_) {
                    if (c.id != d["card"].toString()) continue;
                    if (d.contains("title")) c.title = d["title"].toString();
                    if (d.contains("description")) c.desc = d["description"].toString();
                    if (d.contains("emoji")) c.emoji = d["emoji"].toString();
                    if (d.contains("hidden")) c.hidden = d["hidden"].toBool();
                    c.titleLbl->setText(c.title);
                    c.sub->setText(c.desc);
                }
            }
        }
        if (simpleMode_) { // mode simple : seules les cartes essentielles restent affichées (cartes de plugins comprises)
            static const QStringList advanced = {"models", "ores", "biomes", "decos", "entities", "cmds",
                                                 "pixel", "formspec", "lua", "server", "folder", "plugins"};
            for (Card &c : cards_)
                if (!c.dynamic && advanced.contains(c.id)) c.hidden = true;
        }
        lcApplyPluginStyle(); // couleurs, police et feuilles de style des plugins (toute l'application)
        relayout();
        refreshCardIcons();
        updateCards();
        updateCoreBanner();
    }

    // Ctrl+K : cherche une page ou une action par son nom et l'ouvre (Entrée). Fonctionne aussi en mode simple.
    void openQuickSearch()
    {
        LcDialog dlg(this);
        dlg.setWindowTitle(tr("Aller à..."));
        dlg.resize(460, 400);
        auto *lay = new QVBoxLayout(&dlg);
        auto *edit = new QLineEdit;
        edit->setPlaceholderText(tr("Tapez le nom d'une page ou d'une action, puis Entrée"));
        edit->setClearButtonEnabled(true);
        auto *list = new QListWidget;
        lay->addWidget(edit);
        lay->addWidget(list, 1);
        for (int i = 0; i < int(cards_.size()); ++i) {
            const Card &c = cards_[i];
            auto *it = new QListWidgetItem(c.title + " — " + c.desc);
            it->setData(Qt::UserRole, i);
            list->addItem(it);
        }
        auto firstVisible = [list] {
            for (int i = 0; i < list->count(); ++i)
                if (!list->item(i)->isHidden()) {
                    list->setCurrentRow(i);
                    return;
                }
            list->setCurrentRow(-1);
        };
        connect(edit, &QLineEdit::textChanged, &dlg, [list, firstVisible](const QString &t) {
            for (int i = 0; i < list->count(); ++i)
                list->item(i)->setHidden(!list->item(i)->text().contains(t.trimmed(), Qt::CaseInsensitive));
            firstVisible();
        });
        connect(edit, &QLineEdit::returnPressed, &dlg, [&dlg, list] {
            if (list->currentItem()) dlg.accept();
        });
        connect(list, &QListWidget::itemActivated, &dlg, [&dlg](QListWidgetItem *) { dlg.accept(); });
        firstVisible();
        if (dlg.exec() != QDialog::Accepted || !list->currentItem()) return;
        const int idx = list->currentItem()->data(Qt::UserRole).toInt();
        if (idx >= 0 && idx < int(cards_.size())) cards_[idx].btn->click();
    }

    // Avertissement permanent : tant qu'un plugin autorisé peut modifier LuantiCreator, son comportement peut être altéré.
    void updateCoreBanner()
    {
        if (!coreLbl_) return;
        const QStringList who = plugins().corePlugins();
        if (who.isEmpty()) {
            coreLbl_->hide();
            return;
        }
        coreLbl_->setText(tr("Attention : le comportement de LuantiCreator peut être altéré par un plugin : %1").arg(who.join(", ")));
        coreLbl_->setToolTip(tr("Ces plugins ont l'autorisation de modifier des parties de LuantiCreator (titre, cartes, icônes, style, "
                                "code Lua généré). Si quelque chose se comporte de façon inattendue, désactivez-les dans le gestionnaire de plugins (fenêtre des projets, onglet Plugins)."));
        coreLbl_->show();
    }

    void onPluginsChanged()
    {
        QStringList startupMsgs;
        plugins().runStartup(startupMsgs);
        UiHooks::refresh();
        iconCache_.clear();
        rebuildFeatures();
        refreshCardIcons();
        applyCorePatches();
        if (!startupMsgs.isEmpty()) statusBar()->showMessage(startupMsgs.join(" — "), 12000);
        for (const auto &l : allTabs()) l.second->refreshUi();
        refreshAll();
        statusBar()->showMessage(tr("Plugins mis à jour."), 3000);
    }

    QJsonObject featuresJson()
    {
        QJsonObject o;
        for (auto it = featData_.begin(); it != featData_.end(); ++it) o[it.key()] = it.value();
        for (const auto &f : featTabs_) o[f.first] = f.second->toJson();
        return o;
    }

    // références de blocs/objets de tous les éditeurs (pour détecter les mods utilisés)
    QStringList allRefs()
    {
        QStringList r;
        auto add = [&](const QString &v) {
            for (const QString &x : v.split(',', Qt::SkipEmptyParts)) r << x.trimmed();
        };
        const struct { EditorTab *t; QStringList keys; } src[] = {
            {nodes_, {"drop"}},
            {crafts_, {"output", "r00", "r01", "r02", "r10", "r11", "r12", "r20", "r21", "r22"}},
            {ores_, {"ore", "wherein"}},
            {biomes_, {"node_top", "node_filler", "node_stone"}},
            {decos_, {"deco", "place_on"}}};
        for (const auto &s : src)
            for (const QVariantMap &m : s.t->data())
                for (const QString &k : s.keys) add(m.value(k).toString());
        for (const PluginFeature &f : plugins().features())
            for (const auto &ft : featTabs_)
                if (ft.first == f.key)
                    for (const QVariantMap &m : ft.second->data())
                        for (const Field &fd : f.fields)
                            if (fd.type == TRef || fd.type == TRefList) add(m.value(fd.key).toString());
        return r;
    }

    // dépendances du mod : celles de la page Projet + les mods cités par les blocs (ex : wool:blue -> wool)
    QStringList allDeps(QStringList *added)
    {
        QStringList d;
        for (const QString &x : modDeps_->text().split(',', Qt::SkipEmptyParts))
            if (!identOf(x).isEmpty() && !d.contains(identOf(x))) d << identOf(x);
        const QString mid = identOf(modName_->text());
        for (const QString &r : allRefs()) {
            const int c = r.indexOf(':');
            if (c <= 0) continue;
            const QString m = r.left(c);
            if (m != identOf(m) || m == mid || m == "group" || d.contains(m)) continue;
            d << m;
            if (added) *added << m;
        }
        return d;
    }

    // blocs proposés dans les sélecteurs : éléments du mod, blocs des plugins, blocs de Minetest Game
    QVector<RefEntry> refEntries()
    {
        QVector<RefEntry> v;
        const QString mid = identOf(modName_->text());
        for (EditorTab *t : {nodes_, models_->tab, items_, foods_, tools_})
            for (const QVariantMap &m : t->rawItems()) {
                const QString n = m["name"].toString();
                const QString d = m["description"].toString();
                v.append({mid + ":" + identOf(n), d.isEmpty() ? n : d, "Mon mod", false});
            }
        for (const PluginBlock &b : plugins().blocks()) v.append({b.ref, b.label, b.category, true});
        v += defaultEntries();
        return v;
    }

    // Sélecteur de blocs : recherche, catégories (avec les icônes des plugins), aperçu des textures.
    QString pickBlock(QWidget *parent, const QString &current)
    {
        const QVector<RefEntry> all = refEntries();
        LcDialog dlg(parent ? parent : this);
        dlg.setWindowTitle(tr("Choisir un bloc ou un objet"));
        UiHooks::bindWindow(&dlg, "picker");
        dlg.resize(820, 560);
        auto *v = new QVBoxLayout(&dlg);
        auto *search = new QLineEdit;
        search->setPlaceholderText(tr("Rechercher (nom ou référence)..."));
        search->setClearButtonEnabled(true);
        v->addWidget(search);
        auto *h = new QHBoxLayout;
        auto *cats = new QListWidget;
        cats->setMaximumWidth(210);
        cats->setIconSize(QSize(22, 22));
        auto *grid = new QListWidget;
        grid->setViewMode(QListView::IconMode);
        grid->setIconSize(QSize(44, 44));
        grid->setGridSize(QSize(118, 92));
        grid->setResizeMode(QListView::Adjust);
        grid->setMovement(QListView::Static);
        grid->setWordWrap(true);
        grid->setUniformItemSizes(true);
        h->addWidget(cats);
        h->addWidget(grid, 1);
        v->addLayout(h, 1);
        auto *manual = new QLineEdit(current);
        manual->setPlaceholderText(tr("ou saisissez une référence : mod:nom (ex : wool:blue, default:dirt)"));
        auto *note = new QLabel;
        note->setProperty("muted", true);
        v->addWidget(manual);
        v->addWidget(note);
        auto *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);
        UiHooks::bindBox(bb, "picker");
        v->addWidget(bb);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);

        // catégories dans l'ordre : Tous, Mon mod, catégories rencontrées
        QStringList catNames;
        for (const RefEntry &e : all)
            if (!catNames.contains(e.category)) catNames << e.category;
        auto *allItem = new QListWidgetItem(tr("Tous (%1)").arg(all.size()));
        allItem->setData(Qt::UserRole, QString());
        cats->addItem(allItem);
        for (const QString &c : catNames) {
            int n = 0;
            for (const RefEntry &e : all) n += (e.category == c);
            auto *it = new QListWidgetItem(plugins().categoryIcon(c), QString("%1 (%2)").arg(c).arg(n));
            it->setData(Qt::UserRole, c);
            cats->addItem(it);
        }
        cats->setCurrentRow(0);

        auto fill = [&] {
            grid->clear();
            const QString cat = cats->currentItem() ? cats->currentItem()->data(Qt::UserRole).toString() : QString();
            const QString q = search->text().trimmed().toLower();
            int shown = 0;
            bool more = false;
            QSet<QString> seen;
            for (const RefEntry &e : all) {
                if (!cat.isEmpty() && e.category != cat) continue;
                if (!q.isEmpty() && !e.ref.toLower().contains(q) && !e.label.toLower().contains(q)) continue;
                if (seen.contains(e.ref)) continue;
                seen.insert(e.ref);
                if (shown >= 800) {
                    more = true;
                    break;
                }
                auto *it = new QListWidgetItem(iconFromImage(imageForRef(e.ref), 44), e.label + "\n" + e.ref);
                it->setData(Qt::UserRole, e.ref);
                it->setToolTip(e.ref);
                grid->addItem(it);
                ++shown;
            }
            note->setText(more ? tr("Plus de 800 résultats : affinez la recherche.") : tr("%1 résultat(s)").arg(shown));
        };
        fill();
        connect(search, &QLineEdit::textChanged, &dlg, [&](const QString &) { fill(); });
        connect(cats, &QListWidget::currentRowChanged, &dlg, [&](int) { fill(); });
        connect(grid, &QListWidget::currentItemChanged, &dlg, [&](QListWidgetItem *it) {
            if (it) manual->setText(it->data(Qt::UserRole).toString());
        });
        connect(grid, &QListWidget::itemDoubleClicked, &dlg, [&](QListWidgetItem *it) {
            manual->setText(it->data(Qt::UserRole).toString());
            dlg.accept();
        });
        if (dlg.exec() != QDialog::Accepted) return QString();
        return manual->text().trimmed();
    }

    // met à jour icônes, listes de références et aperçus (textures modifiées ailleurs)
    void refreshAll()
    {
        iconCache_.clear();
        for (const auto &l : allTabs()) {
            l.second->refreshRefs();
            l.second->refreshIcons();
        }
        for (PreviewPane *p : panes_) p->update();
        updateCards();
    }

    void refreshPreview()
    {
        QStringList warns;
        Gen g(t_, identOf(modName_->text()), QString());
        g.setExtra(extraLua(QString(), warns));
        const QString text = g.run(warns);
        QString head;
        for (const QString &w : warns) head += "-- Attention : " + w + "\n";
        if (!head.isEmpty()) head += "\n";
        preview_->setPlainText(head + (text.trimmed().isEmpty() ? QString("-- (rien à générer pour l'instant)\n") : text));
    }

    // ------------------------------------------------------------ dossiers locaux
    // ~/.local/share/LuantiCreator/mods/<id>/        dossier du mod (mod.conf, init.lua, textures/, models/)
    // ~/.local/share/LuantiCreator/projects/<id>.lcproj   données éditables (enregistrées automatiquement)
    static QString modsWorkspace() { return lcModsWorkspace(); }
    static QString projectsDir() { return lcProjectsDir(); }
    static QString projectFile(const QString &id) { return lcProjectFile(id); }
    QString modFolder() const { return modsWorkspace() + "/" + identOf(modName_->text()); }

    static QString freshId()
    {
        QString id = "mon_mod";
        int k = 2;
        while (QFile::exists(projectFile(id))) id = "mon_mod_" + QString::number(k++);
        return id;
    }

    void updatePathLabels()
    {
        const QString p = modFolder();
        modPathLbl_->setText(p);
        statusPathLbl_->setText(p);
        statusPathLbl_->setToolTip(tr("Dossier local du mod : chargé par le serveur de test"));
    }

    void openModFolder()
    {
        if (!sync(true)) return;
        QDesktopServices::openUrl(QUrl::fromLocalFile(modFolder()));
    }

    QJsonObject projectJson()
    {
        QJsonObject o;
        o["name"] = modName_->text();
        o["title"] = modTitle_->text();
        o["desc"] = modDesc_->text();
        o["deps"] = modDeps_->text();
        for (const auto &l : lists_) o[l.first] = l.second->toJson();
        o["features"] = featuresJson();
        o["formspecs"] = formspec_->toJson();
        QJsonArray sn;
        for (const QString &x : pluginPage_->enabledSnippets()) sn.append(x);
        o["snippets"] = sn;
        return o;
    }

    void loadProjectJson(const QJsonObject &o)
    {
        modName_->setText(o["name"].toString());
        modTitle_->setText(o["title"].toString());
        modDesc_->setText(o["desc"].toString());
        modDeps_->setText(o["deps"].toString());
        for (const auto &l : lists_) l.second->fromJson(o[l.first].toArray());
        featData_.clear();
        const QJsonObject fo = o["features"].toObject();
        for (auto it = fo.begin(); it != fo.end(); ++it) featData_[it.key()] = it.value().toArray();
        for (const auto &f : featTabs_) f.second->fromJson(featData_.value(f.first));
        formspec_->fromJson(o["formspecs"].toArray());
        QStringList sn;
        for (const QJsonValue &v : o["snippets"].toArray()) sn << v.toString();
        pluginPage_->setEnabledSnippets(sn);
    }

    // Reconstruit le dossier local du mod à partir du projet (le dossier est entièrement régénéré).
    bool buildLocal(QString &modDir, QStringList &warns, QString &err)
    {
        const QString modid = identOf(modName_->text());
        if (modid.isEmpty()) {
            err = tr("Identifiant du mod manquant (page Projet).");
            return false;
        }
        QDir().mkpath(modsWorkspace());
        modDir = modsWorkspace() + "/" + modid;
        QDir(modDir).removeRecursively();
        return writeMod(modsWorkspace(), warns, err);
    }

    // Enregistre le projet et met à jour le dossier du mod si quelque chose a changé (ou si force).
    bool sync(bool force = false, QStringList *warnsOut = nullptr, QString *errOut = nullptr)
    {
        const QJsonObject o = projectJson();
        const QByteArray snap = QJsonDocument(o).toJson(QJsonDocument::Compact);
        if (!force && snap == lastSnapshot_) return true;

        const QString modid = identOf(modName_->text());
        if (modid.isEmpty()) {
            if (errOut) *errOut = tr("Identifiant du mod manquant (page Projet).");
            return false;
        }
        const bool renamed = !lastBuilt_.isEmpty() && lastBuilt_ != modid;
        if (renamed && QFile::exists(projectFile(modid))) {
            const QString msg = tr("Un autre projet utilise déjà l'identifiant « %1 » : choisissez-en un autre.").arg(modid);
            statusBar()->showMessage(msg, 5000);
            if (errOut) *errOut = msg;
            return false;
        }
        if (renamed) { // l'identifiant a changé : on retire l'ancien dossier et l'ancien projet
            QDir(modsWorkspace() + "/" + lastBuilt_).removeRecursively();
            QFile::remove(projectFile(lastBuilt_));
        }
        QDir().mkpath(projectsDir());
        QFile f(projectFile(modid));
        if (!f.open(QIODevice::WriteOnly)) {
            if (errOut) *errOut = tr("Impossible d'écrire %1").arg(projectFile(modid));
            return false;
        }
        f.write(QJsonDocument(o).toJson());
        f.close();

        QStringList warns;
        QString err, dir;
        if (!buildLocal(dir, warns, err)) {
            statusBar()->showMessage(tr("Erreur : %1").arg(err), 8000);
            if (errOut) *errOut = err;
            return false;
        }
        if (warnsOut) *warnsOut = warns;
        lastSnapshot_ = snap;
        lastBuilt_ = modid;
        {
            QSettings st("LuantiCreator", "LuantiCreator");
            st.setValue("lastProject", projectFile(modid));
        }
        updatePathLabels();
        statusBar()->showMessage(tr("Dossier du mod à jour : %1").arg(dir), 3000);
        return true;
    }

    // ------------------------------------------------------------------ projets
    // Ferme cette fenêtre d'édition et rouvre le lanceur (projets, import de mods, plugins, réglages).
    void backToLauncher()
    {
        QString err;
        if (!sync(true, nullptr, &err)) {
            if (QMessageBox::question(this, tr("Projet non enregistré"),
                                      tr("Le projet n'a pas pu être enregistré :\n%1\n\nQuitter quand même ?").arg(err)) != QMessageBox::Yes)
                return;
        }
        if (AppFlow::showLauncher) AppFlow::showLauncher();
        close(); // arrête aussi le serveur de test (voir closeEvent)
    }

    bool loadProjectFile(const QString &p, bool presync = true)
    {
        QFile f(p);
        if (!f.open(QIODevice::ReadOnly)) return false;
        const QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
        f.close();
        if (o.isEmpty()) return false;
        if (presync) sync(true);
        LcSplash::busy(tr("Chargement du mod"), [&] {
            lastBuilt_.clear();
            loadProjectJson(o);
            lastSnapshot_.clear();
            refreshAll();
            sync(true);
        }, QFileInfo(p).completeBaseName());
        return true;
    }

    // Ctrl+S : le projet est déjà enregistré automatiquement ; on force la mise à jour
    void saveProject()
    {
        QString err;
        if (!sync(true, nullptr, &err)) {
            QMessageBox::warning(this, tr("Erreur"), err);
            return;
        }
        statusBar()->showMessage(tr("Projet enregistré : %1").arg(projectFile(identOf(modName_->text()))), 5000);
    }

    void saveCopy()
    {
        const QString p = QFileDialog::getSaveFileName(this, tr("Enregistrer une copie du projet"),
                                                       QDir::homePath() + "/" + identOf(modName_->text()) + ".lcproj",
                                                       tr("Projet LuantiCreator (*.lcproj)"));
        if (p.isEmpty()) return;
        QFile f(p);
        if (!f.open(QIODevice::WriteOnly)) {
            QMessageBox::warning(this, tr("Erreur"), tr("Impossible d'écrire %1").arg(p));
            return;
        }
        f.write(QJsonDocument(projectJson()).toJson());
        statusBar()->showMessage(tr("Copie enregistrée : %1").arg(p), 5000);
    }

    // Lua des plugins (extraits cochés + fonctionnalités) ; root vide = aperçu sans copie de fichiers
    // Contexte transmis aux scripts des plugins : application, mod et TOUT le projet (pages et fonctionnalités)
    QJsonObject projectContext(const QString &root)
    {
        const QString modid = identOf(modName_->text());
        QJsonObject c, app, mod, project;
        app["name"] = "LuantiCreator";
        app["version"] = LC_VERSION;
        mod["id"] = modid;
        mod["title"] = modTitle_->text();
        mod["description"] = modDesc_->text();
        mod["dir"] = root;
        for (const auto &l : allTabs()) {
            QJsonArray a;
            for (const QJsonValue &v : l.second->toJson()) {
                QJsonObject o = v.toObject();
                const QString ident = identOf(o["name"].toString());
                o["_ident"] = ident;
                o["_ref"] = modid + ":" + ident;
                a.append(o);
            }
            project[l.first] = a;
        }
        project["formspecs"] = formspec_->toJson();
        c["app"] = app;
        c["mod"] = mod;
        c["project"] = project;
        return c;
    }

    // exécute un script « manuel » d'un plugin ; renvoie le texte à afficher (journal, erreurs)
    QString runPluginScript(const QString &pluginId, const QString &scriptId)
    {
        plugins().setContext(projectContext(QString()));
        ScriptResult r;
        QString err;
        const bool ok = plugins().runManual(pluginId, scriptId, r, err);
        QStringList lines;
        for (const QString &l : r.log) lines << l;
        for (const QString &w : r.warnings) lines << "Attention : " + w;
        if (!r.out.trimmed().isEmpty() && !r.usedSdk) lines << r.out.trimmed();
        int added = 0;
        if (ok) {
            for (auto it = r.addItems.begin(); it != r.addItems.end(); ++it) {
                EditorTab *tab = nullptr;
                for (const auto &l : allTabs())
                    if (l.first == it.key()) tab = l.second;
                if (!tab) {
                    lines << tr("Attention : page inconnue « %1 » : éléments ignorés").arg(it.key());
                    continue;
                }
                QJsonArray a = tab->toJson();
                for (const QVariantMap &vm : it.value()) a.append(QJsonObject::fromVariantMap(vm));
                tab->fromJson(a);
                added += it.value().size();
            }
            if (added) refreshAll();
            if (r.coreUsed) { // script « manual » qui modifie LuantiCreator (si le plugin y est autorisé)
                UiHooks::refresh();
                iconCache_.clear();
                applyCorePatches();
            }
            lines << tr("Terminé : %1 élément(s) ajouté(s) au projet.").arg(added);
        } else {
            lines << tr("Erreur : %1").arg(err);
        }
        return lines.join("\n");
    }

    QString extraLua(const QString &root, QStringList &warns)
    {
        plugins().setContext(projectContext(root));
        QMap<QString, QList<QVariantMap>> fd;
        for (const auto &f : featTabs_) fd[f.first] = f.second->data();
        QString lua = plugins().generateLua(pluginPage_->enabledSnippets(), fd, identOf(modName_->text()), root, warns);
        lua += formspec_->toLua(identOf(modName_->text())); // interfaces dessinées dans l'éditeur de formspec
        return lua;
    }

    // écrit parentDir/<modid>/{mod.conf,init.lua,textures,models}
    bool writeMod(const QString &parentDir, QStringList &warns, QString &err)
    {
        const QString modid = identOf(modName_->text());
        if (modid.isEmpty()) {
            err = tr("Identifiant du mod manquant (page Projet).");
            return false;
        }
        const QString root = parentDir + "/" + modid;
        QDir().mkpath(root);
        {
            QFile f(root + "/mod.conf");
            if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                err = tr("Impossible d'écrire dans %1").arg(root);
                return false;
            }
            QTextStream s(&f);
            s.setEncoding(QStringConverter::Utf8);
            s << "name = " << modid << "\n";
            s << "title = " << modTitle_->text() << "\n";
            s << "description = " << modDesc_->text() << "\n";
            QStringList added;
            const QStringList deps = allDeps(&added);
            if (!deps.isEmpty()) s << "depends = " << deps.join(", ") << "\n";
            if (!added.isEmpty())
                warns << tr("Dépendances ajoutées automatiquement d'après vos blocs : %1").arg(added.join(", "));
        }
        QDir().mkpath(root + "/textures");
        Gen g(t_, modid, root);
        g.setExtra(extraLua(root, warns));
        const QString lua = plugins().patchLua(g.run(warns), warns); // luanti_creator_core : modifications du Lua généré
        QFile f(root + "/init.lua");
        if (!f.open(QIODevice::WriteOnly)) {
            err = tr("Impossible d'écrire init.lua");
            return false;
        }
        f.write(lua.toUtf8());
        return true;
    }

    // ------------------------------------------------------------------- export
    struct ExportOpts {
        bool install = false;
        QString installDir;
        bool zip = false;
        QString zipPath;
    };

    bool askExport(ExportOpts &o)
    {
        QSettings st("LuantiCreator", "LuantiCreator");
        const QString modid = identOf(modName_->text());

        LcDialog dlg(this);
        dlg.setWindowTitle(tr("Exporter le mod"));
        UiHooks::bindWindow(&dlg, "export");
        dlg.setProperty("lcHelpTopic", "Exporter");
        dlg.setMinimumWidth(560);
        auto *v = new QVBoxLayout(&dlg);
        auto *intro = new QLabel(tr("Le mod « %1 » est exporté depuis son dossier local :\n%2").arg(modid, modFolder()));
        intro->setWordWrap(true);
        v->addWidget(intro);

        // --- installation dans Luanti
        auto *cInstall = new QCheckBox(tr("Installer dans les mods locaux de Luanti"));
        cInstall->setChecked(st.value("exportInstall", true).toBool());
        auto *dirBox = new QComboBox;
        dirBox->setEditable(true);
        const QStringList dirs = luantiModDirs();
        dirBox->addItems(dirs);
        const QString lastDir = st.value("exportInstallDir").toString();
        dirBox->setCurrentText(lastDir.isEmpty() ? dirs.first() : lastDir);
        auto *bDir = new QPushButton(tr("Parcourir..."));
        UiHooks::bind(bDir, "btn.browse", "dialog.export");
        auto *dirRow = new QHBoxLayout;
        dirRow->addSpacing(24);
        dirRow->addWidget(dirBox, 1);
        dirRow->addWidget(bDir);
        v->addWidget(cInstall);
        v->addLayout(dirRow);
        connect(bDir, &QPushButton::clicked, &dlg, [&dlg, dirBox] {
            const QString d = QFileDialog::getExistingDirectory(&dlg, tr("Dossier « mods » de Luanti"), dirBox->currentText());
            if (!d.isEmpty()) dirBox->setCurrentText(d);
        });

        // --- archive zip
        auto *cZip = new QCheckBox(tr("Créer une archive .zip (à partager ou à installer depuis le menu de Luanti)"));
        cZip->setChecked(st.value("exportZip", false).toBool());
        auto *zipEdit = new QLineEdit(st.value("exportZipPath", QDir::homePath() + "/" + modid + ".zip").toString());
        auto *bZip = new QPushButton(tr("Parcourir..."));
        UiHooks::bind(bZip, "btn.browse", "dialog.export");
        auto *zipRow = new QHBoxLayout;
        zipRow->addSpacing(24);
        zipRow->addWidget(zipEdit, 1);
        zipRow->addWidget(bZip);
        v->addWidget(cZip);
        v->addLayout(zipRow);
        connect(bZip, &QPushButton::clicked, &dlg, [&dlg, zipEdit] {
            const QString p = QFileDialog::getSaveFileName(&dlg, tr("Archive du mod"), zipEdit->text(), tr("Archive zip (*.zip)"));
            if (!p.isEmpty()) zipEdit->setText(p);
        });

        auto *note = new QLabel(tr("Après l'installation, activez le mod dans Luanti : Configurer le monde → cochez le mod."));
        note->setProperty("muted", true);
        note->setWordWrap(true);
        v->addWidget(note);

        auto *bb = new QDialogButtonBox(QDialogButtonBox::Cancel);
        auto *ok = bb->addButton(tr("Exporter"), QDialogButtonBox::AcceptRole);
        ok->setDefault(true);
        UiHooks::bindBox(bb, "export");
        v->addWidget(bb);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        connect(bb, &QDialogButtonBox::accepted, &dlg, [&] {
            if (!cInstall->isChecked() && !cZip->isChecked()) {
                QMessageBox::information(&dlg, tr("Export"), tr("Cochez au moins une option d'export."));
                return;
            }
            dlg.accept();
        });

        if (dlg.exec() != QDialog::Accepted) return false;
        o.install = cInstall->isChecked();
        o.installDir = dirBox->currentText().trimmed();
        o.zip = cZip->isChecked();
        o.zipPath = zipEdit->text().trimmed();
        st.setValue("exportInstall", o.install);
        st.setValue("exportInstallDir", o.installDir);
        st.setValue("exportZip", o.zip);
        st.setValue("exportZipPath", o.zipPath);
        return true;
    }

    void exportMod()
    {
        if (identOf(modName_->text()).isEmpty()) {
            QMessageBox::warning(this, tr("Erreur"), tr("Donnez un identifiant au mod (page Projet)."));
            return;
        }
        ExportOpts o;
        if (!askExport(o)) return;

        QStringList warns;
        QString err;
        if (!sync(true, &warns, &err)) {
            QMessageBox::warning(this, tr("Erreur"), err);
            return;
        }
        const QString modid = identOf(modName_->text());
        const QString src = modFolder();
        QStringList done, problems;

        if (o.install) {
            if (o.installDir.isEmpty()) {
                problems << tr("Aucun dossier d'installation indiqué.");
            } else {
                const QString target = QDir::cleanPath(o.installDir) + "/" + modid;
                bool go = true;
                if (QDir::cleanPath(target) == QDir::cleanPath(src)) {
                    problems << tr("Le dossier choisi est celui de LuantiCreator : choisissez le dossier « mods » de Luanti.");
                    go = false;
                } else if (QDir(target).exists()) {
                    go = QMessageBox::question(this, tr("Mod déjà installé"),
                                               tr("Le mod « %1 » existe déjà dans :\n%2\n\nLe remplacer ?").arg(modid, o.installDir))
                         == QMessageBox::Yes;
                }
                if (go) {
                    QDir(target).removeRecursively();
                    QString cerr;
                    if (copyDir(src, target, &cerr)) done << tr("Installé dans : %1").arg(target);
                    else problems << cerr;
                }
            }
        }
        if (o.zip) {
            QString zp = o.zipPath;
            if (zp.isEmpty()) {
                problems << tr("Aucun chemin d'archive indiqué.");
            } else {
                if (!zp.endsWith(".zip", Qt::CaseInsensitive)) zp += ".zip";
                QString zerr;
                if (writeZip(zp, src, modid, &zerr)) done << tr("Archive créée : %1").arg(zp);
                else problems << zerr;
            }
        }

        QString msg = done.isEmpty() ? tr("Rien n'a été exporté.") : done.join("\n");
        if (!problems.isEmpty()) msg += "\n\n" + tr("Problèmes :") + "\n- " + problems.join("\n- ");
        if (!warns.isEmpty()) msg += "\n\n" + tr("Avertissements :") + "\n- " + warns.join("\n- ");
        if (!done.isEmpty() && o.install) msg += "\n\n" + tr("Activez le mod dans Luanti (Configurer le monde).");
        if (problems.isEmpty()) QMessageBox::information(this, tr("Export terminé"), msg);
        else QMessageBox::warning(this, tr("Export"), msg);
    }

    static inline MainWindow *active_ = nullptr; // fenêtre qui détient les accroches globales (EditorHooks, pctx)
    QStackedWidget *stack_;
    QWidget *home_, *previewPage_ = nullptr;
    QGridLayout *grid_;
    std::vector<Card> cards_;
    QLineEdit *modName_, *modTitle_, *modDesc_, *modDeps_;
    EditorTab *nodes_, *items_, *foods_, *tools_, *crafts_, *ores_, *biomes_, *decos_, *entities_, *cmds_;
    ModelsPage *models_;
    QPlainTextEdit *preview_;
    ServerPage *server_;
    QAction *actStart_, *actRestart_, *actJoin_, *actStop_;
    QLabel *statusLbl_;
    Tabs t_;
    std::vector<std::pair<QString, EditorTab *>> lists_;
    std::vector<PreviewPane *> panes_;
    QHash<QString, QIcon> iconCache_;
    PluginPage *pluginPage_ = nullptr;
    std::vector<std::pair<QString, EditorTab *>> featTabs_;
    std::vector<QWidget *> featPages_;
    QMap<QString, QJsonArray> featData_; // données des fonctionnalités de plugins (conservées si le plugin disparaît)
    QLabel *modPathLbl_ = nullptr, *statusPathLbl_ = nullptr, *coreLbl_ = nullptr;
    FormspecDesigner *formspec_ = nullptr;
    QLabel *hintLbl_ = nullptr; // aide de démarrage de l'accueil (mod vide)
    bool simpleMode_ = false;   // « Mode simple » : masque les cartes avancées
    QTimer *syncTimer_ = nullptr;
    QByteArray lastSnapshot_; // dernier état enregistré (détecte les modifications)
    QString lastBuilt_;       // identifiant du dernier dossier de mod généré
};

// Étapes de démarrage affichées dans le cadre de chargement : thème, plugins, plugins intégrés, scripts de démarrage,
// icônes, textures. Remplit `startupNote` (message de la barre d'état du lanceur).
static QList<LcSplash::Step> startupSteps(QString *startupNote)
{
    QList<LcSplash::Step> steps;
    steps << LcSplash::Step{QObject::tr("Préparation de l'interface"), [](LcSplash &sp) {
        sp.setDetail(QObject::tr("Dossiers de travail et réglages"));
        QDir().mkpath(lcModsWorkspace());
        QDir().mkpath(lcProjectsDir());
        QDir().mkpath(PluginHost::root());
        sp.setStepFraction(0.5);
        lcApplyTheme(QSettings("LuantiCreator", "LuantiCreator").value("dark", true).toBool());
    }, 1};
    steps << LcSplash::Step{QObject::tr("Chargement des plugins"), [](LcSplash &sp) {
        plugins().reload();
        UiHooks::icon = [](const QString &k) { return plugins().icon(k); };
        UiHooks::iconOnly = [](const QString &k) { return plugins().iconOnly(k); };
        sp.setDetail(QObject::tr("%n plugin(s) trouvé(s)", "", plugins().plugins().size()));
    }, 3};
    steps << LcSplash::Step{QObject::tr("Installation des plugins intégrés"), [startupNote](LcSplash &sp) {
        QStringList msgs;
        const int n = plugins().installBundled(&msgs);
        if (!msgs.isEmpty()) *startupNote = msgs.join(" — ");
        else if (n > 0) *startupNote = QObject::tr("%1 plugin(s) intégré(s) installé(s).").arg(n);
        sp.setDetail(n > 0 ? QObject::tr("%n plugin(s) installé(s)", "", n) : QObject::tr("Aucun nouveau plugin intégré"));
    }, 2};
    steps << LcSplash::Step{QObject::tr("Exécution des scripts de démarrage"), [](LcSplash &sp) {
        const QVector<Plugin> &all = plugins().plugins();
        int withScripts = 0;
        for (const Plugin &p : all)
            if (p.enabled && p.hasScripts()) ++withScripts;
        sp.setDetail(withScripts ? QObject::tr("%n plugin(s) avec scripts", "", withScripts) : QObject::tr("Aucun script à lancer"));
        QStringList startupMsgs;
        plugins().runStartup(startupMsgs); // le style des plugins vaut dès le lanceur
        sp.setStepFraction(0.7);
        sp.setDetail(QObject::tr("Application du style des plugins"));
        lcApplyPluginStyle(); // (applique aussi le thème sombre / clair)
    }, 3};
    steps << LcSplash::Step{QObject::tr("Chargement des icônes"), [](LcSplash &sp) {
        QStringList keys;
        for (const Plugin &p : plugins().plugins()) {
            if (!p.enabled) continue;
            for (auto it = p.icons.begin(); it != p.icons.end(); ++it)
                if (!keys.contains(it.key())) keys << it.key();
        }
        int n = 0;
        for (const QString &k : keys) {
            plugins().icon(k); // décode et met l'icône en cache
            if (++n % 8 == 0 || n == keys.size()) {
                sp.setDetail(QObject::tr("%1 / %2 icônes").arg(n).arg(keys.size()));
                sp.setStepFraction(double(n) / keys.size());
            }
        }
        if (keys.isEmpty()) sp.setDetail(QObject::tr("Aucune icône de plugin"));
    }, 2};
    steps << LcSplash::Step{QObject::tr("Chargement des textures"), [](LcSplash &sp) {
        QStringList files;
        for (const Plugin &p : plugins().plugins()) {
            if (!p.enabled) continue;
            for (auto it = p.textures.begin(); it != p.textures.end(); ++it) files << it.value();
            for (const PluginBlock &b : p.blocks)
                if (!b.texture.isEmpty()) files << b.texture;
        }
        files.removeDuplicates();
        int n = 0;
        for (const QString &f : files) {
            QPixmap px;
            if (!QPixmapCache::find(f, &px) && px.load(f)) QPixmapCache::insert(f, px); // textures des plugins prêtes à l'emploi
            if (++n % 16 == 0 || n == files.size()) {
                sp.setDetail(QObject::tr("%1 / %2 textures").arg(n).arg(files.size()));
                sp.setStepFraction(double(n) / files.size());
            }
        }
        if (files.isEmpty()) sp.setDetail(QObject::tr("Aucune texture de plugin"));
    }, 3};
    return steps;
}

int main(int argc, char **argv)
{
#ifdef LC_HAVE_WEBENGINE
    QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts); // requis par Qt WebEngine (documentation de l'API)
#endif
    QApplication app(argc, argv);
    app.setApplicationName("LuantiCreator");
    lcHelpHook() = [](const QString &topic) { ApiDocWindow::open(topic); }; // aide (F1 / bouton « ? ») depuis toutes les fenêtres
    lcApplyTheme(QSettings("LuantiCreator", "LuantiCreator").value("dark", true).toBool()); // police et couleurs dès le cadre de chargement

    // navigation : le lanceur est la fenêtre d'accueil ; ouvrir un mod ferme le lanceur et ouvre l'éditeur du mod
    static QPointer<LauncherWindow> launcher;
    AppFlow::showLauncher = [] {
        if (!launcher) LcSplash::busy(QObject::tr("Ouverture du lanceur"), [] { launcher = new LauncherWindow; });
        AppFlow::lastOpened = launcher;
        launcher->show();
        launcher->raise();
        launcher->activateWindow();
    };
    AppFlow::openProject = [](const QString &path) {
        MainWindow *w = nullptr; // WA_DeleteOnClose : la fenêtre se libère à sa fermeture
        LcSplash::busy(QObject::tr("Ouverture du mod"), [&] { w = new MainWindow(path); },
                       QFileInfo(path).completeBaseName());
        AppFlow::lastOpened = w;
        w->show();
    };
    // Recharge toute l'application : plugins relus, scripts « startup » relancés, style réappliqué, puis les fenêtres
    // principales (lanceur, éditeurs de mod) sont fermées (projets enregistrés, serveur de test arrêté)
    // et rouvertes au même endroit. Le gestionnaire et les éditeurs de plugin restent ouverts et se mettent à jour.
    AppFlow::reloadApp = [] {
        static bool pending = false;
        if (pending) return;
        pending = true;
        QTimer::singleShot(0, [] { // après le traitement du clic en cours (la fenêtre d'origine peut être un éditeur)
            pending = false;
            LcSplash::busy(QObject::tr("Rechargement de l'application"), [] {
            struct Win { QString project; QRect geom; bool max; };
            QVector<Win> mods;
            bool hadLauncher = false;
            QRect launcherGeom;
            QList<QPointer<QWidget>> toClose;
            for (QWidget *w : QApplication::topLevelWidgets()) {
                if (!w->isVisible()) continue;
                if (auto *mw = dynamic_cast<MainWindow *>(w)) {
                    mods.push_back({mw->currentProjectFile(), mw->geometry(), mw->isMaximized()});
                    toClose << mw;
                } else if (auto *lw = dynamic_cast<LauncherWindow *>(w)) {
                    hadLauncher = true;
                    launcherGeom = lw->geometry();
                    toClose << lw;
                }
            }
            for (const auto &w : toClose)
                if (w) w->close();
            QApplication::processEvents();
            QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete); // les fenêtres fermées se libèrent
            plugins().reload();
            QStringList msgs;
            plugins().runStartup(msgs);
            UiHooks::refresh();
            lcApplyPluginStyle();
            for (const Win &mi : mods) {
                if (mi.project.isEmpty() || !AppFlow::openProject) continue;
                AppFlow::openProject(mi.project);
                if (auto *mw = dynamic_cast<MainWindow *>(AppFlow::lastOpened.data())) {
                    if (mi.max) mw->showMaximized();
                    else mw->setGeometry(mi.geom);
                }
            }
            {   // l'application se rouvre sur le lanceur, onglet Plugins
                AppFlow::showLauncher();
                if (auto *lw = dynamic_cast<LauncherWindow *>(AppFlow::lastOpened.data())) {
                    lw->setCurrentTab(1);
                    if (hadLauncher) lw->setGeometry(launcherGeom);
                    lw->statusBar()->showMessage(QObject::tr("Application rechargée."), 5000);
                }
            }
            AppBus::notify(nullptr, "plugins"); // éditeurs de plugin encore ouverts
            });
        });
    };
    // Cadre de chargement (au moins 3 s au lancement) : plugins, textures, icônes... puis le lanceur est construit avant d'être affiché.
    {
        QString note;
        QList<LcSplash::Step> steps = startupSteps(&note);
        steps << LcSplash::Step{QObject::tr("Préparation du lanceur"), [&note](LcSplash &sp) {
            sp.setDetail(QObject::tr("Projets et réglages"));
            LauncherWindow::startupNote = note;
            if (!launcher) launcher = new LauncherWindow;
        }, 4};
        LcSplash splash;
        splash.run(steps, 3000); // 3 s minimum au lancement de l'application
    }
    AppFlow::showLauncher();
    return app.exec();
}

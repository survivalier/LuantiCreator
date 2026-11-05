#pragma once
// GÉNÉRÉ par gen_apidoc.py (via gen_header.py) : ne modifiez pas à la main.
#include <QByteArray>
#include <QString>

namespace lcapidoc {
inline QString markdown()
{
    QByteArray r;
    r.append(R"APIDOC(# Guide de LuantiCreator

Bienvenue dans la **documentation complète de LuantiCreator**, le créateur visuel de mods pour [Luanti](https://www.luanti.org) (anciennement Minetest), dans l'esprit de MCreator.
Cette fenêtre réunit tout ce qu'il faut savoir : le **guide de l'utilisateur** (créer, tester et exporter un mod sans écrire de Lua),
puis le **guide du développeur de plugins** et les **références** (manifeste, icônes, bibliothèques Python).
Utilisez le sommaire de gauche pour naviguer, et le champ de recherche (Ctrl+F ou `/`, Entrée = résultat suivant) pour retrouver un mot.

## À qui s'adresse LuantiCreator ?

- Aux **joueurs** qui veulent ajouter leurs propres blocs, objets, outils ou créatures à Luanti sans apprendre à programmer.
- Aux **créateurs de mods** qui veulent aller plus vite : LuantiCreator écrit le Lua répétitif, vous gardez la main sur le reste.
- Aux **développeurs** qui veulent étendre LuantiCreator lui-même grâce aux **plugins** (nouvelles pages, icônes, thèmes, scripts Python).

## Ce que fait LuantiCreator (et ce qu'il ne fait pas)

LuantiCreator fonctionne par **formulaires** : vous remplissez des champs, un **aperçu en direct** montre le résultat, et le programme **génère le mod complet**
(`mod.conf`, `init.lua`, textures, modèles) dans un dossier local. Vous pouvez ensuite **tester** le mod sur un serveur jetable en un clic, puis l'**exporter**.

| LuantiCreator sait faire | LuantiCreator ne fait pas |
|---|---|
| Blocs, blocs 3D, objets, nourriture, outils | Programmer de la logique libre (callbacks, machines, mécanismes) : utilisez un **plugin** (extraits Lua, scripts) ou complétez le Lua à la main après l'export |
| Recettes (craft, sans forme, cuisson, combustible) | Interpréter du Lua : l'import d'un mod existant ne lit que les définitions écrites en toutes lettres |
| Minerais, biomes, décorations | Remplacer Luanti : le jeu doit être installé séparément pour tester |
| Entités (créatures) avec modèle 3D et comportement simple | Générer des modèles 3D ou des textures animées à votre place (il les importe et les dessine en pixels) |
| Commandes de chat, interfaces (formspec) | |
| Éditeur de pixels, textures animées et superposées | |
| Serveur de test, export `.zip` / installation | |

## Vue d'ensemble du fonctionnement

1. Vous **créez un mod** dans le lanceur (identifiant, titre, dépendances).
2. Dans l'**éditeur de mod**, vous choisissez une **carte** (Blocs, Objets, Recettes…) et vous ajoutez des éléments.
3. Tout est **enregistré automatiquement** et le **dossier du mod est régénéré** à chaque modification.
4. Vous **testez** (serveur de test, F5 puis F6) et vous **exportez** (Ctrl+E).

Toute la génération est **déterministe** : le même projet produit toujours le même mod. Le projet (`.lcproj`) est un fichier JSON lisible que vous pouvez sauvegarder ou partager.

# Installation et lancement

## Prérequis

- **Luanti** (ou Minetest) installé, pour tester les mods : le serveur de test détecte les exécutables `luanti`, `minetest`, `luantiserver` et `minetestserver`.
- Pour **compiler** LuantiCreator : un compilateur C++17, CMake 3.16 ou plus, et Qt6 Widgets. Qt6 WebEngine est facultatif (il fournit le WebView de cette documentation).
- Facultatif : **Python 3** pour les scripts de plugins (et pour régénérer certains fichiers du code source).

## Compilation

```bash
sudo apt install build-essential cmake qt6-base-dev qt6-webengine-dev
./build.sh
./build/LuantiCreator
```

- `qt6-webengine-dev` est facultatif. Sans lui, cette documentation s'affiche dans une fenêtre simple (même contenu, sans coloration du code ni boutons « Copier »).
- `-DLC_WEBVIEW=OFF` force le mode simple ; le WebView n'est pas compatible avec un binaire 100 % statique (`-DLC_STATIC=ON`).
- `build.sh` relance aussi `gen_uikeys.py` (liste des clés d'icônes détectées dans le code) quand Python 3 est disponible.
- Les fichiers `.zip` placés dans le dossier `plugins/` **avant** la compilation sont embarqués dans l'exécutable et installés comme plugins au premier lancement.

## Démarrage : le cadre de chargement

Au lancement, un **cadre de chargement** (splash) s'affiche pendant **au moins 3 secondes**. Il affiche l'icône `app` (celle d'un plugin si un plugin en fournit une),
le nom et la version, l'étape en cours (« Chargement des plugins… »), un compteur « n / N », un détail (« 12 / 40 icônes ») et une barre de progression.
Étapes : dossiers et thème → plugins → plugins intégrés → scripts de démarrage → icônes → textures des plugins → lanceur.
La barre suit le travail réellement accompli sans jamais dépasser le temps écoulé ; le cadre se referme en fondu et le **lanceur** apparaît.

Un cadre plus court (au moins 0,6 s, sans compteur) réapparaît pendant le **chargement d'un mod** et l'**ouverture d'une fenêtre** (éditeur de mod, éditeur de plugin, gestionnaire de plugins, documentation, lanceur recréé, rechargement de l'application).

# Premier mod pas à pas

Ce tutoriel crée un bloc « Rubis », l'ajoute à une recette et le teste en jeu.

1. **Créez le mod.** Dans le lanceur (onglet **Projets**), cliquez sur **Nouveau mod…** (Ctrl+N). Saisissez un identifiant (`rubis_mod`), un titre et gardez `default` en dépendance. Validez : l'éditeur du mod s'ouvre.
2. **Ouvrez la carte « Blocs ».** Cliquez sur **Ajouter** (Ctrl+N). Un bloc apparaît dans la liste, avec son aperçu isométrique à droite.
3. **Réglez le bloc.** Nom technique `rubis_bloc`, nom affiché « Bloc de rubis », choisissez une **texture PNG** (ou dessinez-en une dans l'**Éditeur de pixels**), groupe de minage `cracky`, dureté 2.
4. **Créez une recette.** Carte **Recettes** → Ajouter. Résultat : `rubis_mod:rubis_bloc`. Placez par exemple 9 fois le même ingrédient dans la grille 3×3 (l'aperçu montre la grille de craft).
5. **Testez.** Retournez à l'accueil (Ctrl+H) et cliquez sur **Tester mon mod maintenant** (ou F5 puis F6). LuantiCreator crée un monde jetable, charge le mod et lance Luanti connecté à ce monde.
6. **Exportez.** Ct)APIDOC");
    r.append(R"APIDOC(rl+E : installez le mod dans les mods locaux de Luanti ou créez une archive `.zip` à partager.

Astuce : si votre mod utilise des blocs de **Minetest Game** (`default:stone`…), gardez `default` en dépendance et lancez le serveur avec le jeu **minetest**, pas `devtest`.

# Les fenêtres de LuantiCreator

LuantiCreator ouvre plusieurs fenêtres, chacune avec un rôle précis.

| Fenêtre | Rôle | Comment l'ouvrir |
|---|---|---|
| **Lanceur** | Accueil : projets, plugins, réglages de l'application | Automatique au démarrage ; *Fichier → Gestionnaire de projets* depuis un mod |
| **Éditeur de mod** | Édition d'un mod (cartes, aperçus, serveur de test) | Ouvrir un projet dans le lanceur |
| **Gestionnaire de plugins** | Installer, activer, autoriser, exporter les plugins | Onglet *Plugins* du lanceur ou menu *Plugins → Gérer les plugins* |
| **Éditeur de plugin** | Modifier UN plugin (fichiers, textures, blocs, icônes) | *Modifier le plugin…* ou double-clic dans le gestionnaire |
| **Documentation** | Cette fenêtre | Bouton *Documentation* du lanceur, ou le menu *Aide* de l'éditeur |
| **Boîtes de dialogue** | Nouveau mod, export, sélecteur de blocs, import de textures… | Selon l'action |

**Ouvrir un mod ferme le lanceur** et ouvre l'éditeur du mod. L'inverse existe : *Fichier → Gestionnaire de projets* (Ctrl+Maj+H) enregistre le projet, arrête le serveur de test, ferme l'éditeur et rouvre le lanceur.
Les fenêtres se tiennent à jour entre elles : un plugin modifié dans son éditeur est pris en compte immédiatement par le gestionnaire et par l'éditeur de mod ouvert.
La **taille et la place** des fenêtres principales sont mémorisées.

## Le lanceur

Le lanceur comporte trois onglets.

### Onglet « Projets »

La liste de vos mods (titre, nombre d'éléments, date de modification), une zone de **recherche** (Ctrl+F) et une fiche de détail.

| Bouton | Action |
|---|---|
| **Nouveau mod…** (Ctrl+N) | Crée un projet : identifiant (a-z, 0-9, `_`), titre, description, dépendances. Refuse un identifiant déjà utilisé. |
| **Importer un mod** | Menu : *Un mod Luanti (.zip)…*, *Un dossier de mod…*, *Un projet LuantiCreator (.lcproj)…* |
| **Ouvrir** (Entrée, double-clic) | Ouvre l'éditeur du mod et ferme le lanceur |
| **Dupliquer…** | Copie le projet sous un nouvel identifiant |
| **Exporter en .zip…** | Archive installable du mod |
| **Dossier du mod** | Ouvre le dossier généré dans le gestionnaire de fichiers |
| **Supprimer** (Suppr) | Supprime le projet après confirmation |

### Onglet « Plugins »

Le gestionnaire de plugins (voir *Utiliser des plugins* plus bas) et un bouton **Documentation** qui ouvre cette fenêtre.

### Onglet « LuantiCreator »

- **Apparence** : thème sombre / clair.
- **Dossiers de LuantiCreator** : données, projets, mods générés, textures importées, plugins (bouton *Ouvrir* pour chacun).
- **État** : nombre de projets et de plugins installés / activés ; un **avertissement** apparaît si un plugin peut modifier LuantiCreator.
- **Maintenance** : *Recharger les plugins* (recharge toute l'application), *Réinitialiser les réglages…* (thème, dossiers d'export mémorisés, dernier projet ; vos projets et plugins ne sont jamais touchés), *À propos*.

## L'éditeur de mod

### L'accueil en grille

Chaque **carte** correspond à une fonctionnalité et affiche le nombre d'éléments créés. Le bouton **Tester mon mod maintenant** lance le serveur de test et rejoint la partie.
Les cartes d'action (**Dossier du mod**, **Exporter**, **Enregistrer**, **Projets**) n'ouvrent pas de page. Un mod vide affiche une aide de démarrage.
Les plugins peuvent ajouter leurs propres cartes et changer leurs icônes.

### Les menus

| Menu | Entrées |
|---|---|
| **Fichier** | Accueil du mod (Ctrl+H ou Alt+Gauche) · Gestionnaire de projets (Ctrl+Maj+H) · Enregistrer (Ctrl+S) · Enregistrer une copie sous… · Ouvrir le dossier du mod · Exporter… (Ctrl+E) · Quitter |
| **Affichage** | Thème sombre · Mode simple |
| **Plugins** | Gérer les plugins (nouvelle fenêtre) |
| **Serveur de test** | Lancer (F5) · Relancer avec les modifications (Ctrl+F5) · Rejoindre (F6) · Arrêter (Maj+F5) |
| **Aide** | Documentation (F1) · Aller à… (Ctrl+K) · Premiers pas et raccourcis · À propos |

La **barre d'outils** garde les boutons *Accueil* et *Exporter*, puis ceux du serveur de test (lancer, relancer, rejoindre, arrêter).

### Aller à… (Ctrl+K)

Tapez le nom d'une page ou d'une action (« biomes », « exporter », « rejoindre ») puis Entrée : c'est le moyen le plus rapide de naviguer, y compris vers les pages masquées par le mode simple.

### Mode simple

*Affichage → Mode simple* masque les cartes avancées : **Blocs 3D, Minerais, Biomes, Décorations, Entités, Commandes, Éditeur de pixels, Formspec, Code Lua, Serveur de test, Dossier du mod, Plugins du mod**.
Restent : Projet, Blocs, Objets, Nourriture, Outils, Recettes, Exporter, Enregistrer, Projets (et les cartes ajoutées par des plugins). Tout reste accessible avec Ctrl+K.

### Enregistrement automatique

Le projet est enregistré et le dossier du mod régénéré à chaque modification (environ toutes les 2,5 secondes). **Ctrl+S** force la mise à jour. Renommer le mod renomme son dossier.
**Ne modifiez pas le dossier généré à la main** : il est recréé à chaque mise à jour. Pour garder vos retouches, exportez d'abord.

### Les listes d'éléments

Les pages Blocs, Objets, Nourriture, Outils, Recettes, Minerais, Biomes, Décorations, Entités et Commandes ont la même organisation : une **liste** à gauche, un **formulaire** au centre, un **aperçu en direct** à droite.

| Action | Raccourci |
|---|---|
| Ajouter un élément | Ctrl+N |
| Dupliquer l'élément sélectionné | Ctrl+D |
| Supprimer | Suppr (liste active) |
| Annuler la suppression | Ctrl+Z (liste active) |

Chaque élément possède un **nom technique** (transformé en `a-z0-9_` : « Mon Bloc » devient `mon_bloc`) utilisé dans le Lua, et un **nom affiché** visible en jeu.
Le bouton **Modèles** (visible quand un plugin fournit des modèles prédéfinis))APIDOC");
    r.append(R"APIDOC( les propose en un clic. Les listes affichent l'**icône de texture** de chaque élément.
Les champs de type *fichier* montrent une **miniature** de l'image à côté du chemin.

### Champs « bloc / objet » : références

Beaucoup de champs attendent une **référence** `mod:nom` (par exemple `default:stone` ou `wool:blue`).
Ces champs sont des **listes déroulantes avec icônes** : vos propres éléments, puis les blocs courants de Minetest Game et ceux des plugins (300 au maximum dans la liste déroulante).
Le bouton **…** ouvre le **sélecteur de blocs** : recherche par nom ou référence, catégories, aperçu des textures, et possibilité de saisir directement une référence.
Les mods cités (`wool:…`) sont ajoutés automatiquement aux **dépendances** du mod généré.
Dans un champ référence, une référence sans préfixe désigne votre propre mod.

# Les pages de l'éditeur de mod

## Projet

Identifiant du mod (a-z, 0-9, `_`), titre, description et **dépendances** (séparées par des virgules, par exemple `default, farming`).
La page affiche aussi le **dossier local** du mod et un bouton pour l'ouvrir. L'identifiant devient le préfixe de tout ce que vous créez (`mon_mod:mon_bloc`).

## Blocs

Blocs cubiques ou de forme simple (`core.register_node`).

| Champ | Effet |
|---|---|
| Nom technique / Nom affiché | Identifiant dans le Lua / nom visible en jeu |
| Texture (PNG) | Texture de base. Sans fichier, une texture par défaut (`unknown_node.png`) est utilisée |
| Texture par-dessus (2 maximum) + opacité | Superposition fusionnée dans un seul PNG |
| Animation + paramètres | Bande verticale, planche, durée d'un cycle (voir *Textures animées*) |
| Forme (`drawtype`) | `normal`, `glasslike` (verre), `allfaces` (feuillage), `plantlike` (plante), `torchlike` (torche), `airlike` (invisible) |
| Groupe de minage | `cracky` (pierre), `choppy` (bois), `crumbly` (terre), `snappy` (feuilles), `oddly_breakable_by_hand` |
| Dureté (1-3) | Niveau du groupe |
| Lumière émise (0-14) | `light_source` |
| Solide | Si décoché, on traverse le bloc |
| Taille de pile max | `stack_max` (99 par défaut) |
| Objet lâché | Vide = le bloc lui-même |

**Aperçu** : cube isométrique texturé (verre translucide, plantes à plat, halo de lumière). Les blocs `default:*` sans fichier sont dessinés avec une texture procédurale.

Lua généré (extrait) :

```lua
core.register_node("rubis_mod:rubis_bloc", {
    description = "Bloc de rubis",
    tiles = {"rubis_bloc.png"},
    walkable = true,
    stack_max = 99,
    groups = {cracky = 2},
})
```

## Blocs 3D

Blocs à **modèle 3D** (`drawtype = "mesh"`).

- **Modèle** : `.obj` (conseillé : aperçu filaire et selectionbox complète), `.gltf` / `.glb` (boîte englobante seulement), `.b3d` / `.x` (saisie manuelle des boîtes).
- **Textures 1 à 4** : une par matériau du modèle. Une texture en bande verticale est animée automatiquement.
- **Unités du modèle par node** : échelle.
- **Orientable (facedir)**, lumière, groupe de minage, dureté.
- **Selectionbox** : la zone cliquable du bloc (rouge dans l'aperçu). Bouton **Générer la selectionbox automatiquement** avec cinq modes :
  1. **Boîte englobante** (AABB) ;
  2. **Boîte réduite** (avec une marge) ;
  3. **Voxels** (plusieurs boîtes précises ; résolution 2-64 et nombre max de boîtes) ;
  4. **Cube complet** ;
  5. **Manuelle** (une boîte par ligne : `x1 y1 z1 x2 y2 z2`).
- **Utiliser les boîtes aussi pour la collision** : génère `collision_box`.

L'aperçu 3D se fait pivoter à la souris, avec zoom à la molette. La taille du modèle (en nodes), le nombre de sommets et de triangles sont affichés.

## Objets

Objets d'inventaire (`core.register_craftitem`) : nom, texture (avec superposition), taille de pile.
Luanti n'anime pas l'image d'inventaire d'un objet : la **première image** d'une texture animée est utilisée (avertissement dans la console).
Aperçu : emplacement d'inventaire.

## Nourriture

Comme les objets, avec **Points de vie rendus** (en demi-cœurs : 4 = 2 cœurs). Génère `on_use = core.item_eat(n)`. Aperçu : emplacement d'inventaire et cœurs rendus.

## Outils

Pioches, haches, pelles (`core.register_tool`).

| Champ | Effet |
|---|---|
| Groupe miné | `cracky`, `choppy`, `crumbly`, `snappy` |
| Niveau max (1-3) | `max_drop_level` et `maxlevel` |
| Durabilité | Nombre d'utilisations avant rupture |
| Temps de minage niveaux 1, 2, 3 | En dixièmes de seconde (40 = 4,0 s) |

Aperçu : emplacement d'inventaire avec barre de durabilité.

## Recettes

Quatre types :

| Type | Utilisation |
|---|---|
| `shaped` | Craft avec forme : la disposition dans la grille 3×3 compte |
| `shapeless` | Craft sans forme : seuls les ingrédients comptent |
| `cooking` | Cuisson au four : seul l'ingrédient de la 1ʳᵉ case est utilisé ; durée en secondes |
| `fuel` | Combustible : seul l'ingrédient de la 1ʳᵉ case est utilisé ; durée de combustion |

Champs : nom de la recette, type, **résultat** (une référence), **quantité produite**, durée, et les neuf cases d'ingrédients (ligne × colonne).
Les recettes sans résultat (ou cuisson / combustible sans ingrédient) sont **ignorées** avec un avertissement. Aperçu : grille de craft avec les textures, flèche et résultat.

## Minerais

Génération de minerais (`core.register_ore`) : type `scatter` (amas dispersés) ou `blob` (masses), **bloc du minerai**, **généré dans** (par défaut `default:stone`),
**rareté** (un amas tous les N nodes cubes), minerais par veine, taille de la veine, altitudes min / max. Aperçu : coupe de roche avec une veine du minerai.

## Biomes

Terrains (`core.register_biome`) : bloc de **surface**, de **remplissage**, de **roche**, avec leurs **épaisseurs**, altitudes min / max, **température** et **humidité** (0-100).
La case **Remplacer les biomes et décorations existants** appelle `clear_registered_biomes()` et `clear_registered_decorations()` : seuls vos biomes existeront.
Aperçu : coupe verticale texturée (épaisseurs réelles) et **carte température / humidité** de tous vos biomes. Les noms de biomes sont préfixé)APIDOC");
    r.append(R"APIDOC(s par l'identifiant du mod.

## Décorations

Plantes, arbres simples, décor (`core.register_decoration`, `deco_type = "simple"`) : **bloc placé**, **posé sur** (liste séparée par des virgules),
**biomes** (liste, vide = tous ; les biomes de votre mod sont préfixés automatiquement), **densité** (pour 1000), altitudes. Aperçu : plantes posées sur le sol, densité visible.

## Entités

Créatures (`core.register_entity`).

| Champ | Effet |
|---|---|
| Apparence | `mesh`, `cube`, `sprite`, `upright_sprite` |
| Modèle 3D | Fichier (si apparence = `mesh`) et unités par node |
| Texture | PNG |
| Collisionbox | Automatique d'après le modèle, ou manuelle (`x1 y1 z1 x2 y2 z2`) |
| Points de vie | `hp_max` |
| Comportement | `immobile`, `wander` (erre au hasard, change de direction toutes les 2 à 5 s), `follow` (suit le joueur le plus proche dans un rayon de 12 nodes) |
| Vitesse | Vitesse de déplacement |

Aperçu : modèle 3D filaire avec la collisionbox, plus une fiche de la créature. Les entités sont soumises à la gravité et ont des groupes d'armure par défaut.

## Commandes

Commandes de chat (`core.register_chatcommand`) : nom, paramètres (aide), description, **privilège requis** (vide = aucun) et **message renvoyé** (`{player}` est remplacé par le nom du joueur).
Aperçu : simulation du chat. Pour une logique plus riche, passez par un plugin ou complétez le Lua à la main.

## Éditeur de pixels

Dessinez vos textures directement : clic gauche pour dessiner, clic droit pour gommer, palette de couleurs et **couleur personnalisée**, **Tout effacer**, taille de la grille au choix (8, 16, 32 ou 64 pixels ; 16 par défaut, la taille classique de Luanti),
**Ouvrir un PNG…** et **Enregistrer en PNG…**. Enregistrez dans un dossier connu, puis choisissez le fichier dans le champ *Texture* d'un élément.

## Formspec : concevoir une interface

Dessinez des interfaces de jeu (formspec, `formspec_version[6]`) sans écrire de code.

- **Palette** : texte, bouton, bouton qui ferme, champ de saisie, mot de passe, zone de texte, case à cocher, menu déroulant, image, image d'objet, rectangle coloré, inventaire. Glissez-déposez ou double-cliquez ; « Ajouter du texte » place un texte d'un clic.
- **Canevas** : déplacement à la souris (magnétisme réglable), redimensionnement avec le coin bleu, double-clic pour changer le texte, clic droit (dupliquer, premier plan, arrière-plan, supprimer), Suppr, flèches pour ajuster. La case *Aperçu* masque la grille et les poignées.
- **Propriétés** : position, taille, nom du champ, texte, valeur ; formulaire : nom, dimensions, fond, **commande `/nom`** pour l'ouvrir, Lua exécuté à la réception des champs.
- **Code** : la chaîne formspec et le Lua généré. Les formulaires sont enregistrés dans le projet et ajoutés à `init.lua` (`core.show_formspec`, commande de chat facultative, `core.register_on_player_receive_fields`).
- Annuler / Rétablir : Ctrl+Z / Ctrl+Y (ou Ctrl+Maj+Z) ; les déplacements successifs sont regroupés en une seule étape.

## Code Lua

Affiche le **Lua généré** (`init.lua`) en lecture seule ; **Actualiser** relit le projet. Utile pour apprendre le Lua de Luanti ou vérifier ce qui sera exporté.
Les scripts de plugins ne tournent pas dans cet aperçu : leur code n'apparaît que dans le mod exporté et sur le serveur de test.

## Serveur de test

Teste le mod dans un vrai Luanti, sans toucher à vos mods installés.

| Réglage | Rôle |
|---|---|
| Exécutable Luanti | Détecté automatiquement (*Détecter*) ou choisi à la main (*Parcourir…*) |
| Jeu (gameid) | `minetest` (Minetest Game) ou `devtest`… Si le mod dépend de `default`, utilisez `minetest` |
| Nom du monde de test | Monde jetable dans `~/.local/share/LuantiCreator/worlds/<nom>` |
| Port | Port du serveur local |
| Nom du joueur | Administrateur du serveur |
| Mode créatif / Dégâts | Réglages du monde de test |
| Graine de la carte | Vide = aléatoire |
| Luanti Creator Core | Mod de confort installé dans le monde (décochable) |

Boutons : **Lancer le serveur** (F5), **Relancer avec les modifications** (Ctrl+F5 : ré-export, redémarrage et reconnexion du client), **Arrêter** (Maj+F5), **Rejoindre le serveur** (F6, lance le client connecté à 127.0.0.1), **Réinitialiser le monde**, **Dossier du monde**, **Effacer la console**.
La **console** affiche les journaux du serveur et du client (ERROR en rouge, WARNING en orange).

**Mods supplémentaires** : *Ajouter depuis un ZIP…* installe un mod ou un modpack téléchargé (par exemple depuis ContentDB), *Ajouter un dossier…* fait de même avec un dossier décompressé.
Ils sont rangés dans `~/.local/share/LuantiCreator/server_mods/` ; cochés, ils sont chargés à chaque lancement et survivent à la réinitialisation du monde.
L'archive est lue par LuantiCreator lui-même (pas besoin de `unzip`) : `mod.conf` / `init.lua` sont détectés où qu'ils soient, les chemins dangereux (`../`) sont refusés, le CRC de chaque fichier est vérifié, et les dépendances manquantes sont signalées dans la console.

Commandes utilisées :

```bash
luanti --server --world <monde> --gameid <jeu> --port <port> --config <fichier>
luanti --go --address 127.0.0.1 --port <port> --name <joueur>
```

### Le mod « Luanti Creator Core »

Un mod de test fourni avec LuantiCreator est installé automatiquement dans chaque monde de test. Il ne touche jamais à vos mods exportés. Commandes en jeu :

| Commande | Effet |
|---|---|
| `/status` | Version de LuantiCreator, jeu, mods, joueurs |
| `/lc` ou `/lc help` | Aide |
| `/lc admin` | Donne tous les privilèges |
| `/lc debug` | Console du serveur en direct |
| `/lc mods` | Liste des mods chargés |
| `/lc players` | Liste des joueurs |
| `/lc info` | Informations sur la partie |

## Plugins du mod

Page de l'éditeur de mod qui ne garde que ce qui dépend du mod ouvert : **extraits Lua** cochés (ajoutés à `init.lua`) et **scripts manuels** des plugins. La gestion des plugins se fait dans le lanceur.

# Textures

## Formats et emplacement

- Les textures sont des **PNG**. Pour Luanti)APIDOC");
    r.append(R"APIDOC(, 16 × 16 est la taille classique ; d'autres tailles fonctionnent (32, 64…).
- À la génération, les textures sont **copiées dans le dossier du mod** (`textures/`) ; le Lua y fait référence par leur nom de fichier.
- Sans texture, LuantiCreator utilise une texture par défaut (`unknown_node.png`, `unknown_item.png`).

## Textures superposées

Dans les pages **Blocs**, **Objets**, **Nourriture** et **Outils** : une ou deux **textures par-dessus** avec une **opacité** (0-100 %).
Elles sont posées sur la texture de base (transparence respectée) et **fusionnées en un seul PNG** dans le mod à la génération.
Si la base est animée, la texture par-dessus (d'une seule image) est répétée sur chaque image de l'animation.

## Textures animées

Une texture animée est une image **verticale** dont chaque image est un carré (16 × 64 = 4 images de 16 × 16).

| Mode | Principe |
|---|---|
| **Auto** (défaut) | Détecte une bande verticale d'après l'image |
| **Aucune** | Pas d'animation |
| **Bande verticale** | Vous indiquez le nombre d'images |
| **Planche** | Grille : colonnes × lignes |

Le champ **durée d'un cycle** règle la vitesse. Le Lua utilise `tiles = {{name = ..., animation = {...}}}`. L'aperçu **s'anime en direct** ; les icônes et listes montrent la première image.
Les blocs 3D animent automatiquement une bande verticale. Les objets ne s'animent pas (limite de Luanti).

## Textures procédurales

Les blocs `default:*` sans fichier de texture sont dessinés dans l'aperçu avec une texture procédurale (herbe, pierre, bois…). Ce n'est qu'un aperçu : en jeu, c'est la vraie texture de Minetest Game.

## Import de textures et détection automatique

Dans l'éditeur de plugin, **Importer des textures…** accepte plusieurs images d'un coup et **devine le mod et le bloc** d'après les noms de fichiers :

| Fichier | Bloc deviné |
|---|---|
| `default_dirt.png` | `default:dirt` |
| `default_grass_side.png` | `default:grass` (face « side ») |
| `wool_blue.png` + `wool_red.png` + `farming_wheat.png` | `wool:blue`, `wool:red`, `farming:wheat` |

Avec **plusieurs** fichiers, ce qui se répète donne le nom du mod (y compris les mods à deux mots comme `mcl_core_*`) ; avec **un seul** fichier, le mod est deviné d'après le premier mot (fiable pour les mods connus).
Chaque détection a un niveau de **confiance** (sûr, probable, incertain, sans mod) que vous pouvez corriger avant de valider.

# Tester, exporter, installer

## Tester

- **F5** lance le serveur, **F6** rejoint la partie, **Ctrl+F5** relance après modification, **Maj+F5** arrête.
- Le bouton **Tester mon mod maintenant** (accueil) fait tout d'un coup.
- Le mod est chargé depuis son **dossier local** (copié dans `worldmods`) : vos dossiers de mods ne sont jamais touchés.

## Exporter (Ctrl+E)

La boîte *Exporter le mod* propose, au choix ou les deux :

- **Installer dans les mods locaux de Luanti** : dossiers détectés (`~/.minetest/mods`, `~/.luanti/mods`, `~/.local/share/luanti/mods`, Flatpak…) ou dossier de votre choix. Confirmation avant remplacement d'un mod existant ; LuantiCreator refuse son propre dossier de travail.
- **Créer une archive `.zip`** (dossier racine `<id>/`), installable depuis le menu *Contenu* de Luanti.

À la fin, un résumé liste ce qui a été fait, les **problèmes** et les **avertissements**. Les choix de l'export sont mémorisés. Dans Luanti, pensez à **activer le mod** dans *Configurer le monde*.

## Structure du mod généré

```text
~/.local/share/LuantiCreator/mods/<id>/
├── mod.conf        # name, title, description, depends
├── init.lua        # tout le Lua généré
├── textures/       # PNG copiés (couches fusionnées, animations)
└── models/         # modèles 3D copiés
```

Le projet éditable est dans `~/.local/share/LuantiCreator/projects/<id>.lcproj` (JSON).

## Avertissements courants lors de la génération

| Message | Signification |
|---|---|
| « pas de fichier de modèle : ignoré » | Un bloc 3D ou une entité `mesh` n'a pas de modèle valide |
| « aucune texture, texture par défaut utilisée » | Aucun PNG renseigné |
| « selectionbox générée automatiquement » | Aucune boîte saisie : une boîte englobante a été calculée |
| « Recette … ignorée : pas de résultat / d'ingrédient / de combustible » | Recette incomplète |
| « Minerai / Décoration … ignoré(e) : pas de bloc » | Champ bloc vide |

# Importer un mod existant

Dans le lanceur : **Importer un mod → Un mod Luanti (.zip)** ou **Un dossier de mod**.
LuantiCreator **n'interprète pas le Lua** : il lit les définitions écrites en toutes lettres (`register_node`, `register_craftitem`, `register_tool`) et en tire les **blocs, objets, nourritures et outils**, avec leurs textures (superposées comprises) copiées dans `~/.local/share/LuantiCreator/imports/<id>/textures`.
Le reste du code (callbacks, entités, commandes, machines…) n'est **pas converti** ; le résumé de l'import le rappelle. Conservez donc le mod d'origine pour cette partie.
**Un projet LuantiCreator (.lcproj)** importe simplement un projet partagé.

# Utiliser des plugins

Un **plugin** étend LuantiCreator : icônes, blocs et textures dans les sélecteurs, modèles prédéfinis, extraits Lua, **nouvelles pages d'édition**, scripts Python, et même le style de l'application.

## Installer et gérer

Dans l'onglet **Plugins** du lanceur (ou *Plugins → Gérer les plugins*) :

| Action | Effet |
|---|---|
| **Installer depuis un ZIP…** | Installe un plugin reçu |
| **Nouveau plugin…** | Crée un plugin d'exemple (YAML ou XML) |
| **Créer depuis un mod (.zip)…** | Transforme un mod en plugin (blocs lus dans le Lua, textures importées) |
| Case à cocher | Active / désactive (désactiver annule toutes ses modifications) |
| **Modifier le plugin…** | Ouvre l'éditeur de plugin dans sa propre fenêtre |
| **Exporter en .zip…** | Partager le plugin |
| **Exécuter le script** | Lance un script manuel |
| **Supprimer** | Désinstalle (les données saisies dans ses pages sont conservées si vous le réinstallez) |

Les `.zip` placés dans le dossier `plugi)APIDOC");
    r.append(R"APIDOC(ns/` avant la compilation sont **intégrés à l'exécutable** et installés au premier lancement.

## Autorisations et sécurité

- Un plugin **déclaratif** (niveau 1) n'exécute rien : aucune autorisation requise.
- Les **scripts** ne tournent qu'après **« Autoriser les scripts de ce plugin »** (un script est un vrai programme lancé avec vos droits). Durée maximale : 30 s par script.
- Les modifications de l'application exigent `core: true` **et** « Autoriser ce plugin à modifier LuantiCreator » ; un avertissement permanent est alors affiché.
- Les autorisations sont **liées au contenu** du plugin : elles disparaissent si ses fichiers changent. N'autorisez que des plugins de confiance.

## Éditeur de plugin

Fenêtre dédiée à un plugin (Ctrl+S enregistre, Ctrl+W ferme). Elle permet de modifier le manifeste et les fichiers, **Valider le plugin** (relecture avec numéro de ligne des erreurs, puis rechargement de toute l'application),
**Nouveau fichier…**, **Importer une image…**, **Importer des textures…**, **Importer depuis un mod…**, ajouter un **Bloc…** ou une **Icône…**.
Dans « Icône… », chaque clé d'icône a un **point de couleur** : rouge (icône non enregistrée), orange (couverte par une clé englobante, par exemple `btn.add` couvre `editor.nodes.btn.add`), vert (icône enregistrée).
La liste des clés proposées est **détectée dans le code de l'application** (fichier `uikeys.h`, généré par `gen_uikeys.py`) : toute clé qui existe dans LuantiCreator est reconnue.

La suite de cette documentation décrit en détail le **développement de plugins**.

# Raccourcis clavier

## Éditeur de mod

| Raccourci | Action |
|---|---|
| Ctrl+K | Aller à… (page ou action) |
| Ctrl+H ou Alt+Gauche | Accueil du mod |
| Ctrl+Maj+H | Gestionnaire de projets |
| Ctrl+S | Enregistrer (forcer la mise à jour) |
| Ctrl+E | Exporter |
| F1 | Documentation |
| F5 / Ctrl+F5 / F6 / Maj+F5 | Lancer / relancer / rejoindre / arrêter le serveur de test |

## Listes d'éléments

| Raccourci | Action |
|---|---|
| Ctrl+N | Ajouter un élément |
| Ctrl+D | Dupliquer |
| Suppr | Supprimer (liste active) |
| Ctrl+Z | Annuler la suppression (liste active) |

## Formspec (clavier)

Ctrl+Z annule, Ctrl+Y ou Ctrl+Maj+Z rétablit, Suppr supprime l'élément, les flèches ajustent sa position.

## Lanceur, éditeur de plugin et documentation

| Raccourci | Action |
|---|---|
| Ctrl+N | Nouveau mod (lanceur) |
| Ctrl+F | Rechercher un projet ; Entrée ouvre le résultat |
| Entrée / Suppr | Ouvrir / supprimer le projet sélectionné |
| Ctrl+S / Ctrl+W | Enregistrer / fermer (éditeur de plugin) |
| Ctrl+F ou `/` | Rechercher dans la documentation (Entrée = suivant, Maj+Entrée = précédent, Échap = effacer) |
| Ctrl+molette, Ctrl+ +, Ctrl+ −, Ctrl+0 | Zoom de la documentation |

# Fichiers, dossiers et réglages

| Élément | Emplacement (Linux) |
|---|---|
| Données de l'application | `~/.local/share/LuantiCreator/` |
| Projets (`.lcproj`) | `~/.local/share/LuantiCreator/projects/` |
| Mods générés | `~/.local/share/LuantiCreator/mods/` |
| Textures importées | `~/.local/share/LuantiCreator/imports/` |
| Plugins installés | `~/.local/share/LuantiCreator/plugins/` |
| Mods supplémentaires du serveur de test | `~/.local/share/LuantiCreator/server_mods/` |
| Mondes de test | `~/.local/share/LuantiCreator/worlds/` |

Les **réglages** (thème, mode simple, taille des fenêtres, dernier projet, dossiers d'export, réglages du serveur de test, plugins désactivés) sont enregistrés par Qt (`QSettings`, organisation et application « LuantiCreator »).
*LuantiCreator → Réinitialiser les réglages…* les remet à zéro sans toucher à vos projets.

## Le fichier de projet (.lcproj)

C'est un fichier JSON avec les clés `name`, `title`, `desc`, `deps`, une liste par page (`nodes`, `models`, `items`, `foods`, `tools`, `crafts`, `ores`, `biomes`, `decos`, `entities`, `cmds`), `features` (pages ajoutées par des plugins), `formspecs` et `snippets` (extraits Lua cochés).
Un ancien projet reste valide quand de nouvelles clés apparaissent (toutes sont facultatives).

# Dépannage et questions fréquentes

## Le serveur de test ne démarre pas

- Vérifiez l'**exécutable Luanti** (bouton *Détecter* ou *Parcourir…*) : le message « Aucun exécutable Luanti trouvé dans le PATH » signifie que ni `luanti` ni `minetest` n'est installé ou accessible.
- Vérifiez le **jeu** (`gameid`). Si le mod dépend de `default`, utilisez `minetest` (Minetest Game) ; `devtest` ne le fournit pas.
- Changez de **port** s'il est déjà utilisé. Lisez la console : les erreurs sont en rouge.

## Mon bloc apparaît avec une texture « inconnue »

La texture n'est pas renseignée ou le fichier est introuvable. Choisissez un PNG valide ; la miniature à côté du champ confirme qu'il est lisible.

## Mon mod n'apparaît pas dans Luanti après l'export

Activez-le dans *Configurer le monde* (liste des mods). Vérifiez que le dossier d'installation est bien le dossier `mods` de votre Luanti et que les **dépendances** sont présentes.

## Ma recette ne fonctionne pas

Une recette sans résultat est ignorée. Vérifiez les références `mod:nom` (un `:` est obligatoire) et que les ingrédients existent dans le jeu ou dans un mod listé en dépendance.

## Les biomes ou minerais n'apparaissent pas

La génération s'applique aux **nouvelles zones** de carte : utilisez *Réinitialiser le monde* dans le serveur de test, ou explorez une région neuve. Contrôlez les altitudes (min < max).

## Un plugin signale « clé inconnue »

Les clés d'icônes valides sont celles du code de l'application (`app`, `card.*`, `cat.*`, `btn.*`, `<contexte>.btn.*`, `menu.*`, `tab.*`, `window.*`, `dialog.*`…). Ouvrez *Icône…* dans l'éditeur du plugin pour voir la liste complète.

## L'application est lente à démarrer

Le cadre de chargement dure au moins 3 secondes par conception. Au-delà, beaucoup de plugins, d'icônes ou de textures peuvent allonger l'étape correspondante, visible dans le détail du cadre.

## Je veux revenir à zéro

*LuantiCreator → Réinitialiser les réglage)APIDOC");
    r.append(R"APIDOC(s…* : vos projets et plugins ne sont pas touchés. Pour repartir de rien, supprimez le dossier `~/.local/share/LuantiCreator/` (après sauvegarde).

# Glossaire

| Terme | Définition |
|---|---|
| **Luanti** | Moteur de jeu voxel libre (ancien nom : Minetest) |
| **Mod** | Extension de Luanti : un dossier avec `mod.conf` et `init.lua` |
| **Node / bloc** | Un cube du monde (`register_node`) |
| **Craftitem** | Objet d'inventaire sans placement (`register_craftitem`) |
| **Tool / outil** | Objet qui mine ou frappe (`register_tool`) |
| **Entité** | Objet mobile du monde : créature, projectile |
| **Drawtype** | Manière dont un bloc est dessiné (normal, verre, plante, mesh…) |
| **Groupes** | Étiquettes (`cracky`, `choppy`…) qui disent comment un bloc se mine |
| **Selectionbox / collisionbox** | Boîte cliquable / boîte de collision |
| **Formspec** | Description textuelle d'une interface de jeu |
| **Référence** | Identifiant `mod:nom` d'un bloc ou objet (`default:stone`) |
| **Dépendance** | Mod nécessaire au vôtre (`default`, `farming`…) |
| **Gameid** | Identifiant du jeu (`minetest`, `devtest`) |
| **ContentDB** | Dépôt en ligne des mods Luanti |
| **Plugin** | Extension de LuantiCreator (manifeste YAML ou XML, scripts) |
| **QSS** | Feuille de style Qt, utilisée par les plugins pour changer l'apparence |

# Limites connues

- L'import d'un mod existant ne lit que blocs, objets, nourritures et outils déclarés en toutes lettres.
- Les formats `.b3d` et `.x` n'ont pas d'aperçu : saisissez la selectionbox à la main. Les `.gltf` / `.glb` fournissent seulement une boîte englobante.
- Luanti n'anime pas l'image d'inventaire des objets : la première image est utilisée.
- Les comportements d'entités sont volontairement simples (immobile, errance, suivi) ; pour davantage, utilisez un plugin ou du Lua manuel.
- Le dossier du mod généré est **recréé** à chaque modification : ne le modifiez pas à la main.
- La compilation d'un binaire 100 % statique n'est pas compatible avec le WebView de cette documentation.

# Développer des plugins pour LuantiCreator

Cette partie de la documentation s'adresse à ceux qui veulent **étendre LuantiCreator** : guide pas à pas, référence du manifeste, des
gabarits, des icônes, de la bibliothèque Python `luanti_creator` et du module `luanti_creator_core`. (Le guide de l'utilisateur, pour créer
des mods, se trouve plus haut dans le sommaire.)

## Vue d'ensemble

Un plugin est un **dossier** (ou une archive `.zip`) contenant un manifeste `plugin.yaml` (ou `plugin.xml`) et des fichiers associés.
Il y a trois niveaux de puissance, du plus sûr au plus libre :

| Niveau | Ce que vous écrivez | Ce que ça permet | Autorisation |
|---|---|---|---|
| 1. Déclaratif | `plugin.yaml` / `plugin.xml`, textures, gabarits `.lua` | icônes, blocs et textures dans les sélecteurs, modèles prédéfinis, extraits Lua, **nouvelles pages d'édition** avec leur gabarit | aucune (rien n'est exécuté) |
| 2. Scripts | scripts Python (ou `.sh`, `.js`, `.rb`, `.lua`, `.pl`) avec `import luanti_creator as lc` | générer du Lua par programme, créer des fichiers dans le mod, ajouter des éléments au projet | « Autoriser les scripts de ce plugin » |
| 3. Core | `import luanti_creator_core as core` dans un script | changer le **style** (couleurs, police, QSS), le titre de la fenêtre, les cartes et icônes ; transformer le Lua généré | + « Autoriser ce plugin à modifier LuantiCreator » et `core: true` |

Commencez toujours par le niveau 1 : il suffit pour la plupart des besoins et n'exige aucune confiance particulière.

## Démarrage rapide : un premier plugin en 5 minutes

1. Ouvrez LuantiCreator → onglet **Plugins** → **Nouveau plugin…**, choisissez YAML ou XML. Un plugin d'exemple est créé dans
   `~/.local/share/LuantiCreator/plugins/<id>/`.
2. Double-cliquez dessus (ou **Modifier le plugin…**) : l'éditeur de plugin s'ouvre dans sa propre fenêtre.
3. Remplacez le manifeste par ce plugin minimal, qui ajoute une page « Salutations » :

```yaml
format: 1
id: salutations
name: Salutations
version: "1.0"
author: Vous
description: Ajoute des commandes de chat qui saluent.

features:
  - id: salut
    title: Salutations
    description: Commandes de chat de salutation
    prefix: salut_
    template: templates/salut.lua
    fields:
      - { key: name, label: Nom de la commande, type: text }
      - { key: texte, label: Message, type: text, default: "Bonjour !" }
      - { key: fois, label: Répétitions, type: int, default: 1, min: 1, max: 10 }
      - { key: bruyant, label: En majuscules, type: check }
```

4. Créez `templates/salut.lua` (bouton « Nouveau fichier ») :

```lua
core.register_chatcommand({{name|lua}}, {
    description = "Salue le joueur",
    func = function(player)
        local msg = string.rep({{texte|lua}} .. " ", {{fois|num}})
{{#if bruyant}}
        msg = msg:upper()
{{/if}}
        return true, msg
    end,
})
```

5. Cliquez sur **Valider** : LuantiCreator relit le plugin, signale les erreurs avec le numéro de ligne, puis **recharge toute
   l'application**. Une carte « Salutations » apparaît sur l'accueil d'un mod ; ajoutez-y des éléments, et le Lua généré
   (aperçu, export, serveur de test) contient vos commandes.

## Cycle de développement

- **Valider / Enregistrer** un plugin recharge toute l'application : plugins relus, scripts `startup` relancés, style réappliqué, fenêtres
  du lanceur et des mods rouvertes au même endroit. Vous voyez donc immédiatement le résultat.
- Les problèmes du manifeste (clé inconnue, champ manquant, YAML invalide…) apparaissent sous l'éditeur et dans la fiche du plugin
  (en orange dans la liste).
- Les messages de vos scripts (`lc.log`, `lc.warn`, `lc.error`) sont affichés dans la zone de sortie de l'onglet Scripts et dans la barre d'état.
- Un script Python se teste aussi **hors de LuantiCreator** : `python3 scripts/mon_script.py` (le contexte est alors vide).
- Désactiver la case d'un plugin dans la liste annule toutes ses modifications.
- Pour partager : )APIDOC");
    r.append(R"APIDOC(**Exporter en .zip…** ; pour installer un plugin reçu : **Installer depuis un ZIP…**.

## Tutoriel : un script Python

Un script génère du Lua à partir des éléments d'une fonctionnalité. Dans le manifeste :

```yaml
features:
  - id: des
    title: Dés
    prefix: de_
    script: scripts/des.py
    fields:
      - { key: name, label: Nom, type: text }
      - { key: faces, label: Faces, type: int, default: 6, min: 2, max: 100 }
```

`scripts/des.py` :

```python
import luanti_creator as lc

for it in lc.items:                              # un dict par élément de la page « Dés »
    cmd = it["_ident"]                           # nom technique sûr (a-z0-9_)
    lc.emit("core.register_chatcommand(%s, {" % lc.lua_str("lancer_" + cmd))
    lc.emit("    func = function(name)")
    lc.emit("        return true, tostring(math.random(1, %d))" % int(it["faces"]))
    lc.emit("    end,")
    lc.emit("})")
lc.log("%d dé(s) généré(s)" % len(lc.items))
```

Puis, dans l'onglet Plugins (fiche du plugin), cochez **Autoriser les scripts de ce plugin** (un script est un vrai programme lancé avec
vos droits). Les scripts ne tournent pas dans l'aperçu du code Lua : leur code apparaît dans le mod exporté et sur le serveur de test.

Un script **manuel** peut au contraire écrire dans le projet :

```python
import luanti_creator as lc
lc.add_item("items", {"name": "rubis", "description": "Un rubis", "texture": "default_diamond.png"})
```

## Tutoriel : changer le style de LuantiCreator

Déclarez `core: true` dans le manifeste, ajoutez un script `startup`, puis cochez les **deux** autorisations dans la fiche du plugin.

```yaml
core: true
scripts:
  - id: theme
    name: Mon thème
    file: scripts/theme.py
    event: startup
```

`scripts/theme.py` :

```python
import luanti_creator as lc
import luanti_creator_core as core

if core.allowed:
    core.set_theme_colors(accent="#2e9e6b", accentHover="#3fb97f", bg="#16181c")   # sombre ET clair
    core.set_font(size=11)
    core.add_stylesheet("cartes", """
        QPushButton#card { border-radius: 4px; }
        QTabBar::tab:selected { border-bottom: 3px solid #2e9e6b; }
    """)
else:
    lc.log("modification de LuantiCreator non autorisée")
```

Repères pour écrire du QSS (feuille de style Qt) :

| Sélecteur | Cible |
|---|---|
| `QPushButton#card` | cartes de l'accueil d'un mod |
| `QPushButton[primary="true"]` | boutons principaux (Nouveau mod, Ouvrir, Tester) |
| `QLabel[muted="true"]` | textes secondaires |
| `QLabel[role="title"]`, `QLabel[role="heading"]`, `QLabel[role="warning"]` | titres, sous-titres, avertissement |
| `QTabBar::tab`, `QListWidget::item`, `QLineEdit`, `QComboBox`, `QScrollBar`… | widgets Qt standard |

Les couleurs du thème (`set_theme_colors`) sont reprises par la feuille de LuantiCreator ; vos propres feuilles s'appliquent **après**
et l'emportent. Le style vaut pour toutes les fenêtres : lanceur, éditeur de mod, éditeur de plugin. Le titre de fenêtre et les cartes
concernent la fenêtre d'édition d'un mod.

## Sécurité et autorisations

- Un plugin déclaratif (niveau 1) n'exécute rien.
- Les scripts ne tournent qu'après **« Autoriser les scripts de ce plugin »**. Durée maximale : 30 s par script.
- Les modifications de LuantiCreator (niveau 3) exigent `core: true` **et** « Autoriser ce plugin à modifier LuantiCreator ». Un
  avertissement permanent est alors affiché dans la barre d'état.
- Les autorisations sont liées au contenu du plugin : elles disparaissent si ses fichiers changent (sauf si vous l'éditez vous-même dans
  l'éditeur intégré). N'autorisez que des plugins de confiance.
- Tout ce qu'un plugin demande à LuantiCreator est **validé** : chemins relatifs au plugin, tailles limitées, couleurs `#rrggbb`, opérations connues.

## Identifiants utiles

- **Pages (cartes de l'accueil d'un mod)** : `project nodes models items foods tools crafts ores biomes decos entities cmds pixel formspec lua server folder export save open plugins`, et `plugin.<plugin>.<fonction>` pour les pages ajoutées par un plugin.
- **Pages du projet** (`lc.project`, `lc.add_item`, `lc.names`) : `nodes models items foods tools crafts ores biomes decos entities cmds formspecs`, et `plugin.<plugin>.<fonction>`.
- **Événements de script** : `startup` (lancement et rechargement), `feature` (fonctionnalité), `generate` (génération du mod), `manual` (bouton Exécuter).
- **Couleurs du thème** : `bg surface surface2 border text muted accent accentHover accentText hover scroll danger disabled placeholder`.
- **Variables d'environnement** (autres langages) : `LC_CONTEXT`, `LC_RESULT`, `LC_OUT_DIR`, `LC_PLUGIN_DIR`.

# Référence : manifeste, icônes, scripts


Un plugin est une archive **.zip** (ou un dossier) qui enrichit LuantiCreator :

- **icônes** : cartes de l'accueil, boutons, catégories du sélecteur de blocs, icône de la fenêtre ;
- **blocs et textures** dans tous les sélecteurs de blocs (biomes, minerais, décorations, recettes…) :
  fini la limite à `default:dirt` ou `mon_mod:test`, utilisez `wool:blue`, `farming:wheat`, etc. ;
- **modèles prédéfinis** (un biome « Désert » en un clic…) ;
- **extraits Lua** ajoutés au mod créé ;
- **nouvelles fonctionnalités** : une page d'édition complète (liste + formulaire) avec sa carte sur l'accueil,
  dont le résultat est du Lua produit par un gabarit.

Un plugin est déclaratif : LuantiCreator n'exécute aucun code du plugin **sauf ses scripts** (Python, etc.), qui ne tournent qu'après
autorisation explicite (voir « Scripts »). Les extraits et gabarits Lua sont copiés dans le mod généré (qui, lui, est exécuté par Luanti).
Un plugin peut aussi, avec une seconde autorisation, **modifier des parties de LuantiCreator** (voir « Modifier LuantiCreator »).
N'installez que des plugins de confiance.

### Installer / créer

Accueil → carte **Plugins**.

- **Installer depuis un ZIP…** : l'archive est lue par LuantiCreator, son contenu est affiché pour confirmation.
  Seuls les fichiers `.png .yaml .yml .xml .json .lua .tpl .txt .md .py)APIDOC");
    r.append(R"APIDOC( .sh .js .rb .pl` sont conservés ; chemins dangereux et archives trop grosses refusés.
- **Nouveau plugin…** crée un plugin d'exemple modifiable dans
  `~/.local/share/LuantiCreator/plugins/<id>/`.
- Onglet **Coder un plugin** : éditeur de texte pour le manifeste (`plugin.yaml` ou `plugin.xml`), les `.lua` et gabarits, validation (erreur avec numéro de ligne), boutons
  « Bloc… » (copie la texture et ajoute le bloc au manifeste), « Icône… », « Importer une image… », et **« Importer des textures… »** (voir ci-dessous).
- **Exporter en .zip…** pour partager le plugin.
- Case à cocher = plugin activé / désactivé. Les modifications sont prises en compte immédiatement.

Exemples dans `examples/` : `wool_pack.zip` (15 laines `wool:*` avec textures, catégorie, modèles de biome, extrait Lua),
`flat_icons.zip` (icônes colorées pour l'accueil, les boutons et les catégories) et `script_demo.zip` (scripts Python).

### Importer des textures (détection automatique du mod et du bloc)

Dans l'onglet **Coder un plugin**, « Importer des textures… » ouvre un dialogue où l'on ajoute des images (ou tout un dossier).
Chaque nom de fichier est converti en référence `mod:bloc` :

| Fichier | Référence |
|---|---|
| `default_dirt.png` | `default:dirt` |
| `default_stone_brick.png` | `default:stone_brick` |
| `default_grass_side.png` | `default:grass` (face « side », non importée si `default_grass.png` est aussi présent) |
| `default:cobble.png` | `default:cobble` (référence explicite) |
| `mcl_core_stone.png` + 3 autres `mcl_core_*` | `mcl_core:stone` (mod à deux mots, détecté car ≥ 4 fichiers le répètent) |

Principe : les fichiers sont regroupés par premier mot ; avec plusieurs fichiers, **le préfixe qui se répète est le nom du mod**.
Un fichier seul est plus incertain (indiqué « incertain » en rouge ; « sûr » pour les mods connus comme `default`, `wool`, `farming`…).
**Bloc à plusieurs textures :** la texture de face est préférée (`_front`, puis `_side`, puis la texture sans suffixe, puis `_top`).
**Double-clic sur la case du fichier** : un panneau s'ouvre sous le tableau avec toutes les textures du bloc ; cliquez sur celle à utiliser.
Un fichier sans `_` (`dirt.png`) reçoit le « mod par défaut » du dialogue. Mod, bloc, nom affiché et catégorie restent modifiables avant l'import,
et « Imposer le mod » force un mod pour toutes les lignes. Les blocs sont ajoutés au manifeste ; un bloc déjà déclaré avec la même référence est mis à jour.

### Créer un plugin depuis le .zip d'un mod

Page **Plugins** → « Créer depuis un mod (.zip)… ». LuantiCreator lit l'archive lui-même (rien n'est exécuté) :
- le nom du mod vient de `mod.conf` (sinon du dossier) ; dans un modpack, on choisit le mod à convertir ;
- les blocs sont lus dans les fichiers `.lua` (`register_node("mod:nom", { tiles = {"x.png"} })`), avec leur vraie texture ;
- les textures restantes sont nommées d'après leur fichier (`mod_nom.png` → `mod:nom`), décochées si des blocs Lua ont été trouvés ;
- une texture écrite `"base.png^par_dessus.png"` dans le Lua (par exemple le côté de `default:dirt_with_grass` :
  `default_dirt.png^default_grass_side.png`) est lue en entier : la texture de base et celles posées par-dessus sont
  **fusionnées** dans l'image du bloc (affichée « terre + côté d'herbe » dans le dialogue) ; les modificateurs `^[…]` sont ignorés ;
- pour un bloc à plusieurs textures (`tiles`), la texture de face est choisie (6e entrée, sinon 3e, sinon 1re) ;
  **double-clic sur la case de la texture** ouvre un panneau pour en prendre une autre (celles du bloc, ou toutes celles du mod) ;
- un dialogue permet de cocher / corriger les références avant la création. Le plugin créé est un plugin normal (modifiable, exportable).
**Dans un plugin existant :** onglet « Coder un plugin », bouton **Importer depuis un mod…** (à droite de « Importer des textures… ») :
même dialogue, mais les blocs sont ajoutés au plugin ouvert (un bloc de même référence est mis à jour).
Limite : seuls les noms et textures écrits en toutes lettres dans le Lua sont reconnus (pas ceux construits par code, ex. `modname.."_x.png"`).

### Plugins intégrés à l'exécutable

À la compilation, tous les `.zip` placés dans le dossier **`plugins/`** (à côté de `CMakeLists.txt`) sont embarqués dans
l'exécutable :

```
luanticreator/
├── plugins/
│   ├── mes_icones.zip
│   └── pack_laines.zip
└── CMakeLists.txt
```

Puis `rm -rf build && bash build.sh` (CMake relit le dossier à chaque compilation ; la liste est affichée : « Plugin intégré : … »).
Au lancement, ces plugins sont installés comme n'importe quel plugin (ils apparaissent dans le gestionnaire de plugins avec la mention
« Plugin intégré »), sans rien à installer côté utilisateur. Règles :

- un plugin intégré est réinstallé uniquement quand son archive change (nouvelle compilation avec un `.zip` modifié) ;
- si l'utilisateur le supprime, il ne revient qu'avec une nouvelle version de l'archive ;
- un plugin de même identifiant installé par l'utilisateur n'est jamais écrasé ;
- les **scripts** d'un plugin intégré sont autorisés d'office (ils font partie de votre exécutable) ;
- il reste modifiable, désactivable et exportable comme les autres.

### Structure

    mon_plugin.zip
    └── mon_plugin/            (le dossier racine est facultatif)
        ├── plugin.yaml        (ou plugin.xml)
        ├── icons/  textures/  lua/  templates/

### Le manifeste : `plugin.yaml` ou `plugin.xml`

Un plugin se code en **YAML** ou en **XML** (au choix, équivalents). « Nouveau plugin… » demande le format. L'ancien
`plugin.json` est encore lu pour ne pas casser les plugins existants, mais il n'est plus créé. Si plusieurs manifestes
existent dans le même dossier, l'ordre est : `plugin.yaml`, `plugin.yml`, `plugin.xml`, `plugin.json`.

#### YAML

```yaml
# commentaire
format: 1
id: wool_pack
name: Laines colorées
version: "1.0"          # entre guillemets : sinon 1.0 serait un nombre
author: Vous
description: Pack de laines

icons:
  card.biomes: icons/biomes.)APIDOC");
    r.append(R"APIDOC(png
  btn.add: { file: icons/add.png, icon_only: true }
  cat.laine: icons/laine.png

blocks:
  - ref: "wool:blue"            # les « : » dans une valeur : mettre entre guillemets
    label: Laine bleue
    category: Laine
    texture: textures/blue.png
blocksets:
  - ref: "wool:{name}"
    label: "Laine {name}"
    category: Laine
    texture: "textures/wool_{name}.png"
    names: [white, red, blue]
textures:
  "default:stone": textures/ma_pierre.png

presets:
  - tab: biomes
    name: Prairie
    values:
      node_top: "default:dirt_with_grass"
      heat: 50

snippets:
  - { id: bienvenue, name: Bienvenue, description: "…", file: lua/bienvenue.lua, template: false }

features:
  - id: salutations
    title: Salutations
    description: "…"
    icon: icons/salut.png
    prefix: salut_
    template: templates/salutations.lua
    fields:
      - { key: name, label: Nom, type: text }
      - { key: fois, label: Répétitions, type: int, default: 1, min: 1, max: 10 }
      - key: mode
        type: combo
        options: [a, b]
      - key: aide
        type: text
        hint: |
          Texte sur
          plusieurs lignes
```

Sous-ensemble YAML pris en charge : tables `clé: valeur`, listes `- `, `[a, b]`, `{a: b}`, chaînes `"…"` / `'…'`,
blocs `|` et `>` (avec `-` / `+`), commentaires `#`. Non pris en charge : ancres (`&`, `*`), étiquettes (`!`), plusieurs
documents. Indentez avec des **espaces** (pas de tabulations). Seuls `min`, `max`, `format` et les `values` des modèles
sont lus comme des nombres ; `true` / `false` ne sont des booléens que pour `icon_only`, `template`, `default` et `values`.

#### XML

```xml
<?xml version="1.0" encoding="UTF-8"?>
<plugin format="1" id="wool_pack" name="Laines colorées" version="1.0" author="Vous">
  <description>Pack de laines</description>
  <icons>
    <icon key="card.biomes" file="icons/biomes.png"/>
    <icon key="btn.add" file="icons/add.png" icon_only="true"/>
  </icons>
  <blocks>
    <block ref="wool:blue" label="Laine bleue" category="Laine" texture="textures/blue.png"/>
  </blocks>
  <blocksets>
    <blockset ref="wool:{name}" label="Laine {name}" category="Laine" texture="textures/wool_{name}.png">
      <names><name>white</name><name>red</name><name>blue</name></names>
    </blockset>
  </blocksets>
  <textures><texture ref="default:stone" file="textures/ma_pierre.png"/></textures>
  <presets>
    <preset tab="biomes" name="Prairie">
      <values><value key="node_top">default:dirt_with_grass</value><value key="heat">50</value></values>
    </preset>
  </presets>
  <snippets><snippet id="bienvenue" name="Bienvenue" file="lua/bienvenue.lua" template="false"/></snippets>
  <features>
    <feature id="salutations" title="Salutations" icon="icons/salut.png" prefix="salut_" template="templates/salutations.lua">
      <description>…</description>
      <fields>
        <field key="name" label="Nom" type="text"/>
        <field key="fois" label="Répétitions" type="int" default="1" min="1" max="10"/>
        <field key="mode" type="combo"><options><option>a</option><option>b</option></options></field>
      </fields>
    </feature>
  </features>
</plugin>
```

Règles XML : l'élément racine est `<plugin>` ; les champs simples s'écrivent en attribut **ou** en sous-élément
(`<description>` ci-dessus). Les **listes** `blocks blocksets presets snippets features fields names options` et les
**tables** `icons` (`key` → `file`), `textures` (`ref` → `file`), `values` (`key` → texte) s'écrivent avec leur élément
conteneur. `icon-only` et `icon_only` sont équivalents. Caractères spéciaux : `&amp; &lt; &gt; &quot;` ou `<![CDATA[…]]>`.

Dans les deux formats, seul `id` est obligatoire. Les chemins sont relatifs au dossier du plugin. Quand vous ajoutez un bloc
ou une icône avec les boutons de l'éditeur, le manifeste est réécrit dans son format d'origine (les commentaires ne sont
alors pas conservés).


#### Icônes (`icons`)

| Clé | Où |
|---|---|
| `app` | icône de la fenêtre ; **icône de repli de toutes les fenêtres** |
| `window.<nom>` | icône d'**une** fenêtre ou boîte de dialogue (repli : `app`). Fenêtres : `launcher` (projets), `editor` (édition d'un mod), `pluginedit` (éditeur de plugin), `plugins` (gestionnaire), `apidoc` (documentation). Boîtes de dialogue : `picker` (choix d'un bloc), `export`, `newmod`, `newplugin`, `frommod`, `textures`, `addblock`, `addicon` |
| `dialog.<boîte>.btn.<ok\|cancel\|accept>` | boutons Ok / Annuler / action principale d'une boîte de dialogue (même noms que `window.<nom>` pour les boîtes) ; replis : `dialog.btn.ok`… puis `btn.ok`, `btn.cancel`, `btn.accept` |
| `menu.<action>` | actions des menus : `home projects save savecopy folder export quit dark plugins start restart join stop about close apidoc dup front back del` ; contextes : `launcher.menu.import.zip\|folder\|project`, `pluginedit.menu.save\|close\|apidoc`, `formspec.menu.dup\|front\|back\|del` |
| `card.<page>` | carte de l'accueil : `project nodes models items foods tools crafts ores biomes decos entities cmds pixel lua server folder export save open plugins`, et `card.plugin.<plugin>.<fonction>` pour une fonctionnalité de plugin |
| `btn.<action>` | boutons, partout : `add dup del pick start restart stop join reset openworld zip folder back test install new export reload open import edit about save check newfile image textures modimport block icon browse detect clearlog presets run color clear apidoc prev next selectbox refresh addimages adddir applyall addtext copy ok cancel accept` |
| `<contexte>.btn.<action>` | bouton d'**un seul endroit** (voir ci-dessous) ; l'emporte sur `btn.<action>` |
| `cat.<catégorie>` | catégories du sélecteur de blocs : `mon_mod terrain pierre bois minerais verre objets nourriture`, ou le nom de vos catégories en minuscules (`a-z0-9_`, ex : « Laine » → `cat.laine`) |

**Icônes par contexte.** Un même bouton peut avoir une icône différente selon l'endroit. LuantiCreator cherche la clé la
plus précise, puis les plus générales :
)APIDOC");
    r.append(R"APIDOC(
| Contexte | Où | Exemple |
|---|---|---|
| `home` | accueil | `home.btn.test` : « Tester mon mod maintenant » |
| `toolbar` | barre d'outils du haut | `toolbar.btn.start` `…restart` `…join` `…stop` |
| `server` | page Serveur de test | `server.btn.start` `…restart` `…stop` `…join` `…reset` `…openworld` `…zip` `…folder` `…browse` `…detect` `…del` `…clearlog` |
| `editor` | listes de toutes les pages | `editor.btn.add` `…dup` `…del` `…pick` `…presets` (bouton « Modèles ») |
| `editor.<page>` | liste d'une page | `editor.nodes.btn.add`, `editor.biomes.btn.del`, `editor.plugin.mon_plugin.ma_fonction.btn.add` |
| `plugins` | gestionnaire de plugins | `plugins.btn.install` `…new` `…edit` `…export` `…del` `…folder` `…reload` `…run` |
| `pluginedit` | fenêtre d'édition d'un plugin | `pluginedit.btn.save` `…check` `…newfile` `…image` `…textures` `…modimport` `…block` `…icon` |
| `launcher` | fenêtre des projets (accueil) | `launcher.btn.new` `…import` `…open` `…dup` `…export` `…folder` `…del` `…reload` `…reset` `…about` `…apidoc` |
| `tab` | onglets de la fenêtre des projets | `tab.projects` `tab.plugins` `tab.settings` |
| `page` / `page.<page>` | bouton « Accueil » des pages | `page.btn.back`, `page.server.btn.back` |
| `models`, `preview`, `project`, `pixel`, `apidoc` | boutons de ces pages | `models.btn.selectbox` `models.btn.refresh` `preview.btn.refresh` `project.btn.folder` `pixel.btn.color` `…clear` `…open` `…save` `apidoc.btn.prev` `apidoc.btn.next` |
| `dialog.<boîte>` | boutons d'une boîte de dialogue | `dialog.export.btn.browse` `dialog.textures.btn.addimages` `…adddir` `…clear` `…applyall` `dialog.addblock.btn.pick` `dialog.addicon.btn.pick` |

Exemple : `server.btn.start` → icône du bouton « Lancer le serveur » de la page serveur ; `toolbar.btn.start` → autre icône
pour la barre du haut ; `btn.start` → icône de repli pour les deux. `icon_only` fonctionne avec toutes ces clés.

**Icône seule.** Une icône peut s'écrire sous forme d'objet pour masquer le texte du bouton (ou le titre et la
description d'une carte ; le texte reste visible dans l'info-bulle) :

```yaml
icons:
  btn.add: { file: icons/add.png, icon_only: true }   # objet : icône seule
  btn.del: icons/del.png                              # forme simple
```
(XML : `<icon key="btn.add" file="icons/add.png" icon_only="true"/>`.)

Une fonctionnalité accepte aussi `"icon_only": true` à côté de `"icon"`. La forme simple (chaîne) reste valable.

**Boutons sur une ligne.** Pour un groupe de boutons empilés (liste des éditeurs : Ajouter / Dupliquer / Supprimer ; page
Plugins), dès que **tous** les boutons du groupe ont une icône, ils passent côte à côte sur une seule ligne.
Sinon ils restent en colonne. Combinez avec `icon_only` pour une barre d'icônes compacte.

Plusieurs plugins peuvent fournir la même clé : le premier plugin actif (ordre alphabétique) l'emporte.

**Sélecteur d'icône colorée.** Quand vous codez un plugin, « Icône... » ouvre un sélecteur de clés ; un point de couleur indique l'état de
chaque clé **dans le plugin édité** :

| Couleur | Signification |
|---|---|
| rouge | icône non enregistrée : ni cette clé ni une clé englobante n'a d'icône dans le plugin |
| orange | pas d'icône pour cette clé, mais une **clé englobante** en a une (`btn.add` couvre `editor.nodes.btn.add` ; `editor.btn.add` couvre `editor.nodes.btn.add`) ; l'infobulle donne la clé englobante |
| vert | icône enregistrée directement pour cette clé |

**Clés des pages ajoutées par un plugin.** Une fonctionnalité (`features`) d'identifiant `ma_page` dans le plugin `mon_plugin` crée ces clés :
`card.plugin.mon_plugin.ma_page` (carte de l'accueil), `editor.plugin.mon_plugin.ma_page.btn.add|dup|del` (boutons de la liste),
`page.plugin.mon_plugin.ma_page.btn.back` (bouton « Accueil »). Elles sont proposées **en tête** du sélecteur, et rappelées (avec leur couleur) dans la fiche du plugin
(lanceur → onglet Plugins → Plugins installés). Les clés plus larges `editor.plugin.mon_plugin.btn.add` (toutes les pages du plugin) et `editor.plugin.btn.add` (toutes les pages de plugins) fonctionnent aussi.
Page native ajoutée : `card.formspec`, `editor.formspec.btn.*`, `page.formspec.btn.back`.

#### Blocs, `blocksets` et `textures`

- `blocks` : chaque entrée apparaît dans le sélecteur (bouton « … » ou « + » à côté des champs bloc) avec sa texture
  (PNG, 16×16 conseillé). Les listes déroulantes montrent les 300 premiers blocs de plugins ; le sélecteur les montre tous.
- `blocksets` : génère plusieurs blocs depuis un motif (`{name}` remplacé pour chaque nom de `names`).
- `textures` : image d'aperçu pour n'importe quelle référence existante (par exemple `default:stone`).
- Si un bloc appartient à un autre mod (`wool:blue`), **ce mod est ajouté automatiquement aux dépendances** du mod créé
  (`mod.conf`). Il doit exister dans le jeu utilisé pour tester (Minetest Game contient `wool`).
- Le champ de saisie à côté du sélecteur accepte aussi n'importe quelle référence `mod:nom` écrite à la main.

#### Modèles prédéfinis (`presets`)

`tab` : `nodes models items foods tools crafts ores biomes decos entities cmds`, ou l'`id` d'une fonctionnalité du plugin.
`values` utilise les clés des champs de la page (ex. biomes : `node_top depth_top node_filler depth_filler node_stone ymin ymax heat humid`).
Le menu **Modèles ▾** apparaît sous la liste de la page concernée.

#### Extraits Lua (`snippets`)

Cochés dans Plugins → « Extraits Lua du projet » ; le code est ajouté à la fin de `init.lua`. Avec `"template": true`, le fichier passe
par le moteur de gabarits ci-dessous (variable `modname`).

#### Fonctionnalités (`features`)

Ajoute une carte sur l'accueil et une page d'édition. Chaque élément créé est transmis au gabarit, qui produit du Lua.

Champs (`fields`) : `key` (a-z0-9_), `label`, `type`, et selon le type `default`, `min`, `max`, `options`, `filter`, `hint`.

| type | valeur |
|---|---|
| `text` | texte |
| `int`, `double` | nombre (`min`, `max`) |
| `combo` | choix (`o)APIDOC");
    r.append(R"APIDOC(ptions`) |
| `check` | case à cocher |
| `multi` | texte multiligne |
| `file` | fichier : un PNG est copié dans `textures/`, un autre fichier dans `models/` ; la valeur est le nom du fichier copié |
| `ref` | référence de bloc avec sélecteur |
| `reflist` | liste de références séparées par des virgules |

Un champ `name` (nom technique) est ajouté automatiquement s'il manque.

#### Gabarits

| Syntaxe | Effet |
|---|---|
| `{{champ}}` | valeur brute |
| `{{champ\|lua}}` | chaîne Lua entre guillemets |
| `{{champ\|ident}}` | `a-z 0-9 _` |
| `{{champ\|num}}` / `{{champ\|bool}}` | nombre / `true`/`false` |
| `{{champ\|ref}}` | `mod:nom` (ajoute le mod courant si absent) |
| `{{champ\|upper}}` `lower` `trim` `raw` | transformations (se chaînent : `{{champ\|ref\|lua}}`) |
| `{{#if champ}}…{{#else}}…{{/if}}` | condition (`{{#if !champ}}`, `{{#unless champ}}`) |
| `{{#each champ}}{{this}} {{@index}}{{/each}}` | une itération par ligne d'un champ multiligne |

Variables prédéfinies : `modname`, `name` (identifiant), `name_raw`, `id` (`mod:nom`), `index`.
Une balise de bloc seule sur sa ligne ne laisse pas de ligne vide. Pour une table Lua imbriquée, écrivez `{ {` au lieu de `{{`.

Exemple (`templates/salutations.lua`) :

```lua
core.register_chatcommand({{name|lua}}, {
    func = function(name)
        local msg = string.rep({{texte|lua}} .. " ", {{fois|num}})
{{#if bruyant}}
        msg = msg:upper()
{{/if}}
        return true, msg
    end,
})
```

### Scripts (Python et autres)

Un plugin peut contenir des **scripts** qui génèrent du code à partir des paramètres d'une fonctionnalité, ou qui agissent
sur le projet. Langages : Python (`.py`, interpréteur `python3`), shell (`.sh`), Node (`.js`), Ruby (`.rb`), Lua (`.lua`,
interpréteur `lua`), Perl (`.pl`). L'interpréteur doit être installé sur votre machine.

> **Sécurité.** Un script est un vrai programme lancé avec **vos droits** (fichiers, réseau…) : LuantiCreator ne l'isole pas.
> Les scripts d'un plugin ne tournent que si vous cochez « **Autoriser les scripts de ce plugin** » (gestionnaire de plugins, liste des plugins installés).
> L'autorisation est liée au contenu du plugin : si ses fichiers changent (mise à jour, remplacement), elle disparaît.
> Quand **vous** modifiez votre propre plugin avec l'éditeur intégré, elle est conservée. Durée maximale : 30 s par script.

#### Où déclarer un script

```yaml
features:
  - id: des
    title: Dés
    prefix: de_
    script: scripts/des.py          # génère du Lua à partir des éléments de cette fonctionnalité
    template: templates/x.lua       # facultatif : gabarit habituel, rendu avant le script
    fields: [ ... ]

scripts:
  - id: gemmes
    name: Créer des gemmes
    description: Ajoute des objets au projet
    file: scripts/gemmes.py
    event: manual                   # bouton « Exécuter » de l'onglet Scripts
  - id: signature
    name: Signature
    file: scripts/signature.py
    event: generate                 # à chaque génération du mod (export, test du serveur)
```

XML : `<feature … script="scripts/des.py">` et `<scripts><script id="gemmes" name="…" file="scripts/gemmes.py" event="manual"/></scripts>`.
Option `interpreter` (`python3`, `bash`, `node`…) pour forcer l'interpréteur ; seuls ces interpréteurs sont acceptés.

| Événement | Quand | Ce que le script peut faire |
|---|---|---|
| `feature` (script d'une fonctionnalité) | génération du mod, si la fonctionnalité a des éléments | produire du Lua (`emit`), ajouter des fichiers au mod (`add_file`), avertir |
| `generate` | génération du mod | idem, avec tout le projet en lecture |
| `manual` | bouton « Exécuter » (onglet Scripts) | lire le projet et y **ajouter des éléments** (`add_item`) |

Dans l'**aperçu** du code Lua, les scripts ne sont pas exécutés (un commentaire l'indique) : leur code apparaît dans le mod
exporté et sur le serveur de test.

#### La bibliothèque `luanti_creator` (Python)

Fournie avec LuantiCreator (rien à installer), copie dans `sdk/luanti_creator.py`. Le résultat est transmis automatiquement à la fin du script.

```python
import luanti_creator as lc

for it in lc.items:                         # éléments de la fonctionnalité
    lc.emit("core.register_craftitem(%s, {" % lc.lua_str(it["_ref"]))
    lc.emit("    description = %s," % lc.lua_str(it["name"]))
    lc.emit("})")
lc.log("%d objet(s)" % len(lc.items))
```

| Élément | Rôle |
|---|---|
| `lc.items` | liste des éléments de la fonctionnalité (dicts : un champ = une clé ; `_ident` = nom technique, `_ref` = `mod:nom`) |
| `lc.project` | tout le projet : `{"nodes": [...], "items": [...], "biomes": [...], "plugin.<p>.<f>": [...]}` |
| `lc.mod`, `lc.mod_name` | `{id, title, description, dir}` et l'identifiant du mod |
| `lc.plugin`, `lc.feature`, `lc.app`, `lc.event` | infos du plugin (`id name version dir`), de la fonctionnalité, de LuantiCreator (`version`), de l'événement |
| `lc.emit(code)` | ajoute du Lua à `init.lua` |
| `lc.lua_str(s)` / `lc.lua_value(v)` | chaîne Lua entre guillemets / valeur Python (dict, list, nombre, bool, None) en littéral Lua |
| `lc.ident(s)` | texte → identifiant `a-z0-9_` |
| `lc.render(texte, cle=val)` | remplace `{{cle}}` |
| `lc.add_file("textures/x.png", octets)` | écrit un fichier dans le mod (`.png .jpg .ogg .obj .b3d .x .lua .txt .md .json .conf .tr`, jamais `init.lua`) |
| `lc.copy_file(source, "models/x.obj")` | copie un fichier existant dans le mod |
| `lc.asset("data/x.json")`, `lc.read_asset(...)` | fichier du plugin |
| `lc.names(page)`, `lc.find(page, nom)` | noms des éléments d'une page, recherche d'un élément |
| `lc.add_item(page, {...})` | ajoute un élément à une page (`"items"`, `"nodes"`, `"plugin.<p>.<f>"`) : scripts `manual` |
| `lc.log(m)`, `lc.warn(m)`, `lc.error(m)`, `lc.fail(m)` | journal, avertissement, erreur, erreur + arrêt |

Les clés d'un élément sont celles des champs de la page (`name`, `description`, `texture`, `stack`… pour « Objets » ;
`key` des champs pour une fonctionnalité de plugin).)APIDOC");
    r.append(R"APIDOC(

#### Événement `startup`

```yaml
scripts:
  - id: theme
    name: Mon thème
    file: scripts/theme.py
    event: startup        # au lancement de LuantiCreator et à chaque rechargement des plugins
```
Il sert aux modifications de l'interface (ci-dessous). Il n'a pas de projet à lire (`lc.project` est vide) et ne produit pas de Lua.

#### Modifier LuantiCreator (`luanti_creator_core`)

Le style d'un plugin autorisé vaut pour **toutes les fenêtres** (lanceur, éditeur de mod, éditeur de plugin). Dans une feuille de style, les
widgets utiles sont `QPushButton#card` (cartes de l'accueil), `QPushButton[primary="true"]` (boutons principaux), `QLabel[muted="true"]`
(textes secondaires), `QLabel[role="title"]` / `[role="heading"]`, plus tous les widgets Qt habituels (`QTabBar::tab`, `QListWidget::item`…).

**Rechargement.** Valider (`Valider`), enregistrer, recharger, activer / désactiver un plugin ou lui accorder une autorisation **recharge toute
l'application** : plugins relus, scripts `startup` relancés, style réappliqué, puis le lanceur et les fenêtres d'édition de mod sont refermés (projets
enregistrés, serveur de test arrêté) et rouverts au même endroit. Les éditeurs de plugin restent ouverts.

Pour donner plus de liberté aux développeurs, un script Python peut **modifier des parties existantes de LuantiCreator** avec un second import :

```python
import luanti_creator as lc
import luanti_creator_core as core          # <- ce second import demande une autorisation supplémentaire

if core.allowed:                            # False tant que l'utilisateur n'a pas autorisé le plugin
    core.set_window_title("Mon atelier")
    core.set_card("biomes", title="Terrains", emoji="⛰")
    core.hide_card("pixel")
    core.set_icon("btn.add", "icons/plus.png", icon_only=True)
    core.set_theme_colors(accent="#2e9e6b", bg="#16181c")
    core.set_font(size=11)
    core.add_stylesheet("cartes", "QPushButton#card { border-radius: 4px; }")
    core.lua_append("-- ajouté par mon plugin")
```

| Fonction | Effet | Événements |
|---|---|---|
| `set_window_title(texte)` | titre de la fenêtre | `startup`, `manual` |
| `set_card(id, title=, description=, emoji=, hidden=)`, `hide_card(id)`, `show_card(id)` | carte de l'accueil (`project nodes … plugins`, `formspec`, `plugin.<plugin>.<fonction>`) | `startup`, `manual` |
| `set_icon(cle, fichier, icon_only=False)` | icône d'une clé, **prioritaire** sur celles des plugins (fichier du plugin, `.png .jpg .bmp`) | `startup`, `manual` |
| `set_stylesheet(qss)` | feuille de style Qt (QSS) de toute l'application, appliquée **après** celle de LuantiCreator (64 Ko max) | `startup`, `manual` |
| `add_stylesheet(nom, qss)` | feuille de style **nommée** : un même nom remplace la précédente, des noms différents se cumulent | `startup`, `manual` |
| `set_theme_colors(accent=, bg=, surface=, …)` | couleurs du thème (sombre et clair), au format `#rrggbb`. Noms : `bg surface surface2 border text muted accent accentHover accentText hover scroll danger disabled placeholder` | `startup`, `manual` |
| `set_font(family=, size=)` | police de l'application (taille de 6 à 32 pt) | `startup`, `manual` |
| `lua_prepend(code)`, `lua_append(code)`, `lua_replace(ancien, nouveau, count=0)` | modifie l'`init.lua` généré | `generate`, `feature` |

**Règles et sécurité**
- Le manifeste doit déclarer **`core: true`** (XML : `<plugin … core="true">`). Sans cela, les demandes sont ignorées et signalées.
- Il faut **deux autorisations**, jamais accordées d'office (même pour un plugin intégré) : « Autoriser les scripts de ce plugin », puis
  « Autoriser ce plugin à modifier LuantiCreator ». Cette seconde case affiche un **avertissement** : le comportement de LuantiCreator peut être altéré à cause du plugin.
  Les autorisations sont liées au contenu du plugin : elles disparaissent si ses fichiers changent (sauf quand vous le modifiez vous-même dans l'éditeur intégré).
- **Avertissement permanent** : tant qu'un plugin autorisé est actif, la barre d'état affiche « Attention : le comportement de LuantiCreator peut être altéré par un plugin : … »,
  et un `init.lua` modifié commence par un commentaire `-- Attention : ce fichier a été modifié par le(s) plugin(s) : …`.
- Sans autorisation, `import luanti_creator_core` fonctionne (`core.allowed` vaut `False`) mais toutes les demandes sont ignorées avec un avertissement.
- Les modifications sont des **demandes** transmises à LuantiCreator, qui les valide (chemins relatifs au plugin, tailles limitées, opérations connues). Elles sont annulées
  quand le plugin est désactivé ou ses scripts non autorisés ; un script `startup` est relancé à chaque rechargement des plugins.
- Un script « manual » peut modifier l'interface ; les modifications du Lua se font dans les scripts `generate` / de fonctionnalité (pas dans l'aperçu du code Lua, où les scripts ne tournent pas).

Exemple : `examples/core_demo.zip`.

#### Autres langages

Sans la bibliothèque, le protocole est simple : variables d'environnement `LC_CONTEXT` (fichier JSON du contexte : `event`,
`app`, `mod`, `plugin`, `feature`, `items`, `project`), `LC_RESULT` (fichier JSON de résultat à écrire :
`{"lua": "...", "log": [], "warnings": [], "errors": [], "add_items": {"items": [{...}]}}`), `LC_OUT_DIR` (fichiers à ajouter au mod),
`LC_PLUGIN_DIR`. **Sans fichier résultat, la sortie standard du script est prise comme code Lua** (ex : un script `.sh` qui fait `echo`).
Un code de sortie non nul est signalé comme une erreur.

Exemple complet : `examples/script_demo.zip` (fonctionnalité « Dés » générée par Python, script manuel « Créer des gemmes »,
script de fin de génération).

### Éditeur de formspec

La carte **Formspec** de l'accueil permet de dessiner des interfaces de jeu par glisser-déposer ; elles sont enregistrées dans le projet
(clé `formspecs`) et leur code Lua est ajouté à `init.lua`. Les scripts reçoivent aussi ces formulaires : `lc.project["formspecs"]`
(liste de `{name, w, h, bg, command, on_fields, )APIDOC");
    r.append(R"APIDOC(elems: [{type, x, y, w, h, name, text, extra, flag}]}`).

### Données du projet

Les éléments des fonctionnalités et les extraits cochés sont enregistrés dans le projet (`.lcproj`). Si le plugin est
désinstallé, ses données sont conservées et réapparaissent s'il est réinstallé.

# Référence de l'API Python : luanti_creator

Bibliothèque Python fournie avec LuantiCreator (rien à installer) : `import luanti_creator as lc`. Variables du module : `lc.context` (contexte brut), `lc.event` (`feature`, `generate`, `manual` ou `startup`), `lc.app`, `lc.mod`, `lc.mod_name`, `lc.plugin`, `lc.feature`, `lc.items`, `lc.project`, `lc.core_allowed`, `lc.API_VERSION`.

## Présentation du module

```text
luanti_creator : bibliothèque pour les scripts Python des plugins LuantiCreator.

    import luanti_creator as lc

    for it in lc.items:                 # éléments saisis dans la fonctionnalité du script
        lc.emit("core.register_craftitem(%s, {" % lc.lua_str(lc.mod_name + ":" + it["_ident"]))
        lc.emit("    description = %s," % lc.lua_str(it["name"]))
        lc.emit("})")

Rien à initialiser : le résultat est transmis à LuantiCreator automatiquement à la fin du script.
Lancé à la main (hors de LuantiCreator), le script fonctionne aussi avec un contexte vide.

Pour aller plus loin (modifier des parties existantes de LuantiCreator : titre, cartes, icônes, style, code Lua généré),
un second import existe : `import luanti_creator_core as core`. Il demande une autorisation supplémentaire de
l'utilisateur et LuantiCreator affiche alors un avertissement : son comportement peut être altéré par le plugin.
```

## Fonctions

### `lc.emit(code='', end='\n')`

Ajoute du code Lua au mod (init.lua).

### `lc.lua_str(s)`

Chaîne Python -> littéral de chaîne Lua entre guillemets.

### `lc.lua_value(v, indent=0)`

Valeur Python -> littéral Lua (None, bool, nombres, str, list -> table, dict -> table).

### `lc.ident(s)`

Texte -> identifiant technique (a-z, 0-9, _), comme dans LuantiCreator.

### `lc.render(template, **values)`

Remplace {{cle}} dans un texte (substitution simple, pas de logique).

### `lc.log(msg)`

Message d'information (console de LuantiCreator).

### `lc.warn(msg)`

Avertissement affiché avec ceux de la génération.

### `lc.error(msg)`

Erreur : la génération continue mais le script est signalé en échec.

### `lc.fail(msg)`

Signale une erreur et arrête le script.

### `lc.add_file(rel, data)`

Écrit un fichier dans le dossier du mod (ex : "textures/mon_bloc.png", "sounds/x.ogg").  
`data` : bytes ou str. Disponible quand le mod est généré (événements "feature" et "generate").

### `lc.copy_file(src, rel)`

Copie un fichier existant dans le mod (ex : copy_file(item["fichier"], "models/x.obj")).

### `lc.asset(rel)`

Chemin absolu d'un fichier du plugin (images, données...).

### `lc.read_asset(rel, binary=False)`

Contenu d'un fichier du plugin.

### `lc.names(page)`

Noms (champ « name ») des éléments d'une page du projet.

### `lc.find(page, name)`

Premier élément de la page dont le nom (ou l'identifiant) vaut `name`, sinon None.

### `lc.add_item(page, values)`

Ajoute un élément à une page du projet ("nodes", "items", "biomes", "plugin.<plugin>.<fonction>"...).  
Il apparaît dans LuantiCreator à la fin du script. Surtout utile pour les scripts « manuels ».

# Référence de l'API Python : luanti_creator_core

Module `import luanti_creator_core as core` : modifie des parties de LuantiCreator. Exige `core: true` dans le manifeste et l'autorisation « Autoriser ce plugin à modifier LuantiCreator ». `core.allowed` vaut `True` si le plugin est autorisé. Les fonctions d'interface fonctionnent dans les événements `startup` et `manual` ; les fonctions `lua_*` dans `generate` et `feature`.

## Présentation du module

```text
luanti_creator_core : modifier des parties EXISTANTES de LuantiCreator depuis un plugin (usage avancé).

    import luanti_creator as lc
    import luanti_creator_core as core

    core.set_window_title("Mon atelier")
    core.set_card("biomes", title="Terrains", emoji="⛰")
    core.hide_card("pixel")
    core.set_icon("btn.add", "icons/plus.png", icon_only=True)
    core.set_theme_colors(accent="#2e9e6b", bg="#16181c")
    core.set_font(size=11)
    core.add_stylesheet("boutons", "QPushButton { border-radius: 14px; }")
    core.lua_append("-- fin du mod, ajouté par mon plugin")

AVERTISSEMENT
-------------
Importer ce module peut modifier le comportement de LuantiCreator. Il ne fonctionne que si l'utilisateur a
explicitement autorisé le plugin à « modifier LuantiCreator » (page Plugins) ; sans cette autorisation, toutes les
demandes sont ignorées et un avertissement est affiché. Tant qu'un plugin autorisé est actif, LuantiCreator affiche en
permanence : « Le comportement de LuantiCreator peut être altéré par un plugin ». Utilisez-le avec parcimonie et
documentez ce que fait votre plugin.

Quand les modifications sont appliquées :
  - événement « startup » (au lancement et à chaque rechargement des plugins) et scripts « manual » :
    fenêtre, cartes, icônes, style (couleurs, police, QSS) ;
  - événements « generate » et « feature » : code Lua généré (lua_prepend / lua_append / lua_replace).
```

## Fonctions

### `core.set_window_title(text)`

Change le titre de la fenêtre principale.

### `core.set_stylesheet(qss)`

Ajoute une feuille de style Qt (QSS) à toute l'application (64 Ko max), appliquée APRÈS celle de LuantiCreator.  
Elle remplace la feuille « principale » précédente du plugin (voir add_stylesheet pour en avoir plusieurs).

### `core.add_stylesheet(name, qss)`

Ajoute une feuille de style NOMMÉE (64 Ko max). Un même nom remplace la feuille précédente ; des noms  
différents se cumulent. Pratique pour découper un style en plusieurs morceaux (boutons, listes, onglets...).

### `core.set_theme_colors(**colors)`

Change les couleurs du thème (sombre ET clair), au format "#rrggbb". Noms possibles :  
  bg (fond de fenêtre), surface (champs, listes, cartes), surface)APIDOC");
    r.append(R"APIDOC(2 (boutons, survol des cartes), border (bordures),  
  text (texte), muted (texte secondaire), accent (couleur d'accent), accentHover, accentText (texte sur l'accent),  
  hover (survol des éléments), scroll (barres de défilement), danger, disabled, placeholder.  
Ces couleurs sont utilisées dans les feuilles de style : `core.add_stylesheet` peut aussi les reprendre à la main.

### `core.set_font(family=None, size=None)`

Change la police de l'application (`family` : nom de police, `size` : taille en points, 6 à 32).

### `core.set_card(card_id, title=None, description=None, emoji=None, hidden=None)`

Modifie une carte de l'accueil (identifiants : project nodes models items foods tools crafts ores biomes decos  
entities cmds pixel formspec lua server folder export save open plugins, ou plugin.<plugin>.<fonction>).  
Seuls les paramètres donnés sont changés.

### `core.hide_card(card_id)`

Masque une carte de l'accueil.

### `core.show_card(card_id)`

Réaffiche une carte masquée.

### `core.set_icon(key, rel_file, icon_only=False)`

Fournit l'icône d'une clé (voir PLUGINS.md : card.*, btn.*, cat.*...), en PRIORITÉ sur les icônes des  
plugins. `rel_file` : image du plugin (chemin relatif à son dossier).

### `core.lua_prepend(code)`

Ajoute du Lua au DÉBUT de l'init.lua généré (événements generate / feature).

### `core.lua_append(code)`

Ajoute du Lua à la FIN de l'init.lua généré (événements generate / feature).

### `core.lua_replace(old, new, count=0)`

Remplace `old` par `new` dans l'init.lua généré (count = 0 : toutes les occurrences).
)APIDOC");
    return QString::fromUtf8(r);
}
} // namespace lcapidoc

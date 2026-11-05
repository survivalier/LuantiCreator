# Guide de LuantiCreator

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
6. **Exportez.** Ctrl+E : installez le mod dans les mods locaux de Luanti ou créez une archive `.zip` à partager.

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
Le bouton **Modèles** (visible quand un plugin fournit des modèles prédéfinis) les propose en un clic. Les listes affichent l'**icône de texture** de chaque élément.
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
Aperçu : coupe verticale texturée (épaisseurs réelles) et **carte température / humidité** de tous vos biomes. Les noms de biomes sont préfixés par l'identifiant du mod.

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

- Les textures sont des **PNG**. Pour Luanti, 16 × 16 est la taille classique ; d'autres tailles fonctionnent (32, 64…).
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

Les `.zip` placés dans le dossier `plugins/` avant la compilation sont **intégrés à l'exécutable** et installés au premier lancement.

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

*LuantiCreator → Réinitialiser les réglages…* : vos projets et plugins ne sont pas touchés. Pour repartir de rien, supprimez le dossier `~/.local/share/LuantiCreator/` (après sauvegarde).

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

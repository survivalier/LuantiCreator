# LuantiCreator 0.9

Créateur visuel de mods pour [Luanti](https://www.luanti.org), dans l'esprit de MCreator.
Qt6 Widgets / C++17. Aucune dépendance autre que Qt6 Widgets (pas d'OpenGL, pas de Qt3D).

## Compilation
    sudo apt install build-essential cmake qt6-base-dev qt6-webengine-dev
    ./build.sh
    ./build/LuantiCreator

`qt6-webengine-dev` (Qt WebEngine) fournit le **WebView** de la « Documentation de l'API » (voir plus bas). Il est facultatif : s'il manque,
CMake le signale et la documentation s'affiche dans l'ancienne fenêtre simple (même contenu, sans coloration ni boutons « Copier »).
`-DLC_WEBVIEW=OFF` force ce mode simple. Le WebView n'est pas compatible avec un binaire 100 % statique (`-DLC_STATIC=ON`).

Fichiers sources à garder ensemble : `main.cpp appcore.h codehl.h winframe.h launcher.h modimport.h editor.h preview.h server.h fsutil.h zipread.h inflate.h generator.h mesh.h widgets.h corepack.h plugins.h pluginpage.h uihooks.h template.h markup.h scriptrun.h lcsdk.h texturedetect.h texcompose.h formspec.h apidoc.h apidocwindow.h apidocview.h splash.h uikeys.h CMakeLists.txt`.
Les bibliothèques Python des plugins (`luanti_creator.py`, `luanti_creator_core.py`) sont embarquées dans `lcsdk.h` ; après les avoir modifiées (ou `PLUGINS.md` / `apidoc_intro.md`, source de la fenêtre « Documentation de l'API ») : `python3 gen_header.py`.

## Démarrage : cadre de chargement
Au lancement, un **cadre de chargement** s'affiche (au moins 3 secondes ; un cadre plus court s'affiche aussi à l'ouverture d'une fenêtre ou d'un mod, voir `LcSplash::busy`) pendant que LuantiCreator se prépare : dossiers et thème, plugins,
plugins intégrés, scripts de démarrage des plugins, icônes, textures des plugins, puis construction du lanceur. La barre suit le travail
réellement accompli (étape en cours, détail comme « 12 / 40 icônes », pourcentage). Quand tout est prêt, il se referme en fondu et
le **lanceur** apparaît. Code : `splash.h` (classe `LcSplash`) et `startupSteps()` dans `main.cpp` : pour ajouter une étape, ajoutez un
`LcSplash::Step{"Libellé", [](LcSplash &sp) { ...; sp.setDetail("..."); sp.setStepFraction(0.5); }, poids}`.

## Documentation complète (WebView)
*Aide → Documentation de LuantiCreator* (F1, ou le bouton **Documentation** du lanceur) ouvre la **documentation complète** : guide de l'utilisateur (`guide_utilisateur.md` : installation, premier mod, chaque page de l'éditeur, textures, serveur de test, export, import, plugins, raccourcis, dépannage, glossaire), puis guide du développeur de plugins (`apidoc_intro.md`, `PLUGINS.md`) et références Python, dans un **WebView** (Qt WebEngine, aucun accès réseau :
tout est embarqué dans l'exécutable) :
- mise en page sobre qui suit le thème sombre / clair de LuantiCreator (et les couleurs d'un plugin autorisé) ;
- **coloration du code** (yaml, lua, python, xml, json, bash, javascript), gabarits `{{champ|lua}}` mis en évidence ;
- **icône « Copier »** sur chaque bloc de code (retour « Copié ») ; un clic sur un code en ligne (`lc.add_item`) le copie aussi ;
- **sommaire** à gauche (filtrable, replié par niveaux, section courante mise en évidence), **recherche** surlignée
  (Ctrl+F ou `/`, Entrée = suivant, Maj+Entrée = précédent, Échap = effacer), bouton « haut de page » ;
- zoom Ctrl+molette / Ctrl+ + / Ctrl+ − / Ctrl+0 ; les liens externes s'ouvrent dans le navigateur du système.
Le contenu est généré par `python3 gen_header.py` (`apidoc.h`, à partir de `guide_utilisateur.md`, `apidoc_intro.md`, `PLUGINS.md` et des docstrings Python) ; la page elle-même (HTML/CSS/JS) est dans `apidocview.h`.

## Cadre des fenêtres
Toutes les fenêtres (lanceur, éditeur de mod, éditeur et gestionnaire de plugins, documentation) et les boîtes de dialogue propres à LuantiCreator ont leur **propre cadre**, dans le style du cadre de chargement :
fenêtre sans bordure système, **entièrement opaque** (aucune transparence), **coins arrondis** et fine bordure, **barre de titre** avec favicon (l'icône de la fenêtre, y compris celle d'un plugin), titre en gras et boutons **réduire / agrandir-restaurer / fermer**.
Glisser la barre déplace la fenêtre, un double-clic l'agrandit, les bords et les coins la redimensionnent ; agrandie, la fenêtre devient carrée. Les couleurs suivent le thème (et les plugins).
Code : `winframe.h` (`LcChrome::install(fenêtre)` pour un `QMainWindow` / `QDialog`, `LcDialog` pour une boîte de dialogue). Les boîtes du système (`QMessageBox`, sélecteurs de fichiers) gardent le cadre natif.
**Aide partout** : chaque fenêtre a un bouton **?** dans sa barre de titre et répond à **F1** ; la documentation s'ouvre sur la section qui correspond (lanceur, éditeur de mod, export), et reste utilisable pendant une boîte de dialogue. La fenêtre de documentation n'a pas de bouton « ? » (propriété `lcNoHelp`) ; pour une section précise : `setProperty("lcHelpTopic", "Titre de section")`. F1 fonctionne aussi avec les cadres natifs.
Pour retrouver les cadres natifs : `LC_NATIVE_FRAMES=1` ou `customFrames=false` dans les réglages ; coins carrés : `kRadius = 0` dans `winframe.h`.

## Coloration du code
Tous les éditeurs et visualiseurs de code sont colorés (`codehl.h`, classe `LcHighlighter`) : code Lua généré (page « Code Lua »), extraits Lua des plugins,
formspec et Lua générés par l'éditeur de formspec, et **éditeur de fichiers de plugin** (langage choisi d'après l'extension : `.lua`, `.py`, `.yaml`/`.yml`, `.json`,
`.xml`/`.html`, `.conf` / `settingtypes.txt`, `.md`, `.sh`). Les couleurs suivent le thème sombre / clair, y compris quand on bascule le thème avec des fenêtres ouvertes.
Pour colorer un nouvel éditeur : `lcAttachHighlighter(edit, LcLang::Lua);`. Pour un nouveau langage : voir le commentaire en tête de `codehl.h`.

## Ergonomie et raccourcis
- **Ctrl+K « Aller à… »** (éditeur de mod) : tapez le nom d'une page ou d'une action, Entrée l'ouvre. **Mode simple** (Affichage) : n'affiche que l'essentiel ;
  le reste reste accessible par Ctrl+K. L'accueil d'un mod vide explique par où commencer (Aide → Premiers pas et raccourcis).
- **Listes d'éléments** (blocs, objets, recettes, etc.) : Ctrl+N ajoute, Ctrl+D duplique, Suppr supprime, **Ctrl+Z (liste active) annule la suppression**.
- **Éditeur de formspec** : Ctrl+Z / Ctrl+Y (ou boutons Annuler / Rétablir) ; les déplacements successifs sont regroupés en une étape.
- **Lanceur** : Entrée ouvre le mod, Suppr le supprime (avec confirmation), Ctrl+N crée, Ctrl+F cherche (Entrée dans la recherche ouvre le résultat).
- La barre d'outils de l'éditeur garde **Accueil** et **Exporter** ; Alt+Gauche = accueil du mod ; la taille et la place des fenêtres (lanceur, éditeur) sont mémorisées.

## Fenêtres : lanceur, éditeur de mod, éditeur de plugin
Au démarrage, LuantiCreator n'ouvre **pas** directement l'éditeur : il ouvre le **lanceur**, avec trois onglets.
- **Projets** : liste de vos mods (titre, nombre d'éléments, date), recherche, et
  **Nouveau mod…**, **Importer un mod** (archive `.zip`, dossier de mod, ou projet `.lcproj`), **Ouvrir** (ou double-clic),
  **Dupliquer…**, **Exporter en .zip…**, **Dossier du mod**, **Supprimer**.
  L'import d'un mod `.zip` / dossier lit ses blocs, objets, nourritures et outils (`register_node`, `register_craftitem`,
  `register_tool`), copie leurs textures (superposées comprises) et crée un projet. Le reste du code du mod n'est pas converti.
- **Plugins** : le gestionnaire (installer un .zip, nouveau plugin, créer depuis un mod, activer, autoriser les scripts,
  exporter, supprimer). **Modifier le plugin…** (ou double-clic) ouvre l'**éditeur de plugin dans sa propre fenêtre**.
- **LuantiCreator** : thème sombre / clair, ouverture des dossiers de données, état des plugins (avertissement si un plugin
  peut modifier LuantiCreator), rechargement des plugins, réinitialisation des réglages, à propos.

**Ouvrir un mod ferme le lanceur et ouvre la fenêtre d'édition du mod.** Depuis l'éditeur : *Fichier → Gestionnaire de projets*
(Ctrl+Maj+H, ou la carte « Projets ») ferme l'éditeur (le projet est enregistré, le serveur de test arrêté) et rouvre le lanceur ;
*Plugins → Gérer les plugins* ouvre le gestionnaire dans une fenêtre à part. La page « Plugins du mod » de l'éditeur ne garde que
ce qui dépend du mod ouvert : extraits Lua cochés et scripts manuels. Les fenêtres se tiennent à jour entre elles : un plugin
modifié dans son éditeur est pris en compte tout de suite par le gestionnaire et par l'éditeur de mod ouvert.

## Dossier du mod, serveur de test et export
Quand vous créez un mod, LuantiCreator crée son dossier **en local** :

    ~/.local/share/LuantiCreator/mods/<id>/             dossier du mod (mod.conf, init.lua, textures/, models/)
    ~/.local/share/LuantiCreator/projects/<id>.lcproj   projet éditable

- Le projet est **enregistré automatiquement** et le dossier du mod est **régénéré** à chaque modification
  (toutes les ~2,5 s). Renommer le mod renomme son dossier.
- Le **serveur de test** met à jour ce dossier puis le charge dans son monde (copie dans `worldmods`).
  Les mods de votre Luanti ne sont jamais touchés.
- **Exporter** (Ctrl+E) propose, au choix ou les deux :
  - **Installer dans les mods locaux de Luanti** (dossiers détectés : `~/.minetest/mods`, `~/.luanti/mods`,
    `~/.local/share/luanti/mods`, Flatpak...), avec confirmation avant remplacement ;
  - **Créer une archive .zip** (dossier racine `<id>/`, installable depuis le menu Contenu de Luanti).
- Menu Fichier : ouvrir le dossier du mod, enregistrer une copie du projet ailleurs.

## Interface
- **Accueil en grille** : une carte par fonctionnalité, avec le nombre d'éléments créés.
- **Aperçu en direct** à droite de chaque éditeur :
  - Blocs : cube isométrique texturé (verre translucide, plantes à plat, halo de lumière)
  - Objets / Nourriture / Outils : emplacement d'inventaire, cœurs rendus, durabilité
  - Recettes : grille de craft 3×3 avec les textures des ingrédients, flèche et résultat (cuisson / combustible aussi)
  - Minerais : coupe de roche avec une veine du minerai
  - Biomes : **coupe verticale avec les textures** (surface / remplissage / roche, épaisseurs réelles) et
    **carte température/humidité** montrant tous vos biomes
  - Décorations : plantes posées sur le sol choisi, densité visible
  - Entités : modèle 3D filaire avec la collisionbox, fiche de la créature
  - Commandes : simulation du chat
- **Icônes de textures** dans toutes les listes ; champs « bloc / objet » = listes déroulantes avec icônes
  (vos éléments + blocs courants de Minetest Game).
- Miniature de la texture à côté de chaque champ fichier. Thème sombre / clair (menu Affichage).
- Les blocs `default:*` sans fichier sont dessinés avec une texture procédurale pour l'aperçu.

## Éditeur de formspec (carte « Formspec »)
Dessinez des interfaces de jeu (formspec, `formspec_version[6]`) sans écrire de code :
- **Glissez-déposez** des éléments depuis la palette (ou double-clic) : texte, bouton, bouton qui ferme, champ de saisie, mot de passe, zone de texte,
  case à cocher, menu déroulant, image, image d'objet, rectangle coloré, inventaire. « Ajouter du texte » place un texte d'un clic.
- Le canevas est l'**aperçu** : déplacez à la souris (magnétisme réglable), redimensionnez avec le coin bleu, double-clic pour changer le texte,
  clic droit (dupliquer, premier plan, arrière-plan, supprimer), Suppr, flèches. La case « Aperçu » masque la grille et les poignées.
- Panneau **Propriétés** : position, taille, nom du champ, texte, valeur ; formulaire : nom, dimensions, fond, commande `/nom`, Lua exécuté à la réception des champs.
- Onglet **Code** : la chaîne formspec et le Lua généré. Les formulaires sont enregistrés dans le projet et **ajoutés à `init.lua`** automatiquement
  (`core.show_formspec`, commande de chat facultative, `core.register_on_player_receive_fields`).

## Textures animées et textures superposées
Dans les pages **Blocs** (et **Objets / Nourriture / Outils** pour la superposition) :
- **Texture par-dessus** (2 maximum) + **opacité** : posée sur la texture de base, transparence respectée. Les couches sont
  fusionnées dans un seul PNG du mod à la génération. Si la base est animée, la texture par-dessus (d'une seule image)
  est répétée sur chaque image de l'animation.
- **Animation** (blocs) : une texture animée est une image verticale dont chaque image est un carré (16×64 = 4 images de 16×16).
  Modes : *Auto* (détectée d'après l'image, par défaut), *Aucune*, *Bande verticale* (nombre d'images), *Planche* (colonnes × lignes).
  Le champ « durée d'un cycle » règle la vitesse. Le Lua généré utilise `tiles = {{name = ..., animation = {...}}}`.
- L'aperçu **s'anime en direct** ; les icônes et les grilles montrent la première image.
- Blocs 3D : une texture en bande verticale est animée automatiquement. Objets : Luanti n'anime pas l'image d'inventaire,
  la première image est utilisée (avertissement dans la console).
- Les textures animées des plugins s'affichent avec leur première image.

## Serveur de test (carte « Serveur de test », barre d'outils, F5 / Ctrl+F5 / F6)
- Détecte `luanti` / `minetest` / `luantiserver` / `minetestserver` et les jeux installés (devtest, minetest...).
- Crée un monde jetable dans `~/.local/share/LuantiCreator/worlds/<nom>` et y charge le mod (depuis son dossier
  local, copié dans `worldmods`) à chaque lancement : vos dossiers de mods ne sont pas touchés.
- **Lancer**, **Relancer avec les modifications** (ré-export + redémarrage, reconnexion automatique du client),
  **Arrêter**, **Rejoindre** (lance le client Luanti connecté à 127.0.0.1 avec le nom choisi), réinitialiser le monde.
- **Mods supplémentaires** : « Ajouter depuis un ZIP... » installe un mod (ou un modpack) téléchargé, par exemple
  depuis ContentDB, sur le serveur de test ; « Ajouter un dossier... » fait de même avec un mod déjà décompressé.
  Les mods sont rangés dans `~/.local/share/LuantiCreator/server_mods/`, cochés = chargés à chaque lancement
  (ils survivent à la réinitialisation du monde). L'archive est lue par LuantiCreator lui-même (pas besoin de `unzip`) :
  `mod.conf` / `init.lua` sont détectés où qu'ils soient dans l'archive, les chemins dangereux (`../`) sont refusés,
  le CRC de chaque fichier est vérifié, et les dépendances non fournies sont signalées dans la console.
- **Luanti Creator Core** : un mod de test fourni avec LuantiCreator est installé automatiquement dans chaque monde de test
  (option décochable dans la page du serveur). Commandes en jeu : `/status` (version de LuantiCreator, jeu, mods, joueurs), `/lc` (aide), `/lc admin` (tous les privilèges), `/lc debug` (console du serveur en direct), `/lc mods`, `/lc players`, `/lc info`. Il ne touche jamais à vos mods exportés.
- Console avec les logs du serveur et du client (ERROR en rouge, WARNING en orange).
- Réglages : jeu, port, nom du joueur (administrateur), mode créatif, dégâts, graine de la carte.
- Si votre mod dépend de `default`, utilisez le jeu « minetest » (Minetest Game), pas « devtest ».

Commandes Luanti utilisées :
`luanti --server --world <monde> --gameid <jeu> --port <port> --config <fichier>` puis
`luanti --go --address 127.0.0.1 --port <port> --name <joueur>`.

## Plugins (.zip)
Onglet **Plugins** du lanceur : installez un plugin depuis un .zip, ou créez et codez le vôtre (éditeur intégré, validation, export .zip). Un plugin peut aussi contenir des **scripts Python** (génération de code, ajout d'éléments au projet) avec une bibliothèque `luanti_creator` fournie, après autorisation explicite. Les `.zip` placés dans le dossier `plugins/` avant la compilation sont **intégrés à l'exécutable** comme plugins. Le manifeste d'un plugin s'écrit en **YAML** (`plugin.yaml`) ou en **XML** (`plugin.xml`), plus en JSON.
Un plugin peut fournir des **icônes** (cartes, boutons, catégories), des **blocs avec textures** dans les sélecteurs
(biomes, minerais, décorations, recettes : `wool:blue`, `farming:wheat`… au lieu de se limiter à `default:dirt`),
des **modèles prédéfinis**, des **extraits Lua** et de **nouvelles fonctionnalités** (pages d'édition + gabarits Lua).
**Importer des textures…** (éditeur de plugin, onglet « Coder un plugin ») : importez plusieurs images d'un coup, le mod et le bloc sont devinés d'après les noms de fichiers (`default_dirt.png` → `default:dirt`).
**Créer depuis un mod (.zip)…** (gestionnaire de plugins) : transforme le .zip d'un mod en plugin (blocs lus dans le Lua, textures importées).
Le bouton **…** à côté des champs bloc ouvre un sélecteur avec recherche, catégories et aperçu des textures.
Les mods cités par vos blocs (`wool:…`) sont ajoutés automatiquement aux dépendances.
**Icônes d'un plugin** : dans « Icône... », chaque clé a un point de couleur — rouge : icône non enregistrée, orange : couverte par une clé englobante
(`btn.add` couvre `editor.nodes.btn.add`), vert : icône enregistrée. Les clés des pages ajoutées par le plugin y sont proposées en tête et rappelées dans sa fiche.
**Modifier LuantiCreator depuis Python** : `import luanti_creator_core` (titre, cartes, icônes, style, code Lua généré), avec autorisation explicite de l'utilisateur
et un avertissement permanent : le comportement de LuantiCreator peut être altéré par un plugin. Voir `PLUGINS.md`.
Format complet : `PLUGINS.md` ; exemples : `examples/wool_pack.zip`, `examples/flat_icons.zip`.

## Blocs 3D et selectionbox automatique
Importez un `.obj` (aperçu filaire), un `.gltf`/`.glb` (boîte englobante) ou un `.b3d`/`.x` (saisie manuelle),
puis « Générer la selectionbox automatiquement » :
1. Boîte englobante (AABB) · 2. Boîte réduite · 3. Voxels (multi-boîtes précises) · 4. Cube complet · 5. Manuelle.
Les boîtes (rouge) sont modifiables à la main et servent aussi de collision_box si l'option est cochée.

## Un seul exécutable
Par défaut libstdc++ et libgcc sont liées statiquement et Qt reste dynamique. Pour un binaire autonome :
Qt6 statique + `-DLC_STATIC=ON`, ou AppImage via `linuxdeploy --plugin qt`.

## Clés d'icônes détectées dans le code
La validation des icônes de plugin ne se fie pas à une liste écrite à la main : `gen_uikeys.py` lit le code (`UiHooks::bind`, `bindWindow`, `bindBox`, `bindRow`, `addAct(..., "menu.xxx")`) et génère `uikeys.h` (`build.sh` le relance à chaque compilation si Python 3 est disponible). Les clés déclarées à l'exécution par les fenêtres ouvertes (`UiHooks::seenKeys()`) s'y ajoutent. Une clé qui existe dans l'application n'est donc jamais signalée « inconnue ».

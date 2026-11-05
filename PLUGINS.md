# Plugins LuantiCreator

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

## Installer / créer

Accueil → carte **Plugins**.

- **Installer depuis un ZIP…** : l'archive est lue par LuantiCreator, son contenu est affiché pour confirmation.
  Seuls les fichiers `.png .yaml .yml .xml .json .lua .tpl .txt .md .py .sh .js .rb .pl` sont conservés ; chemins dangereux et archives trop grosses refusés.
- **Nouveau plugin…** crée un plugin d'exemple modifiable dans
  `~/.local/share/LuantiCreator/plugins/<id>/`.
- Onglet **Coder un plugin** : éditeur de texte pour le manifeste (`plugin.yaml` ou `plugin.xml`), les `.lua` et gabarits, validation (erreur avec numéro de ligne), boutons
  « Bloc… » (copie la texture et ajoute le bloc au manifeste), « Icône… », « Importer une image… », et **« Importer des textures… »** (voir ci-dessous).
- **Exporter en .zip…** pour partager le plugin.
- Case à cocher = plugin activé / désactivé. Les modifications sont prises en compte immédiatement.

Exemples dans `examples/` : `wool_pack.zip` (15 laines `wool:*` avec textures, catégorie, modèles de biome, extrait Lua),
`flat_icons.zip` (icônes colorées pour l'accueil, les boutons et les catégories) et `script_demo.zip` (scripts Python).

## Importer des textures (détection automatique du mod et du bloc)

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

## Créer un plugin depuis le .zip d'un mod

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

## Plugins intégrés à l'exécutable

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

## Structure

    mon_plugin.zip
    └── mon_plugin/            (le dossier racine est facultatif)
        ├── plugin.yaml        (ou plugin.xml)
        ├── icons/  textures/  lua/  templates/

## Le manifeste : `plugin.yaml` ou `plugin.xml`

Un plugin se code en **YAML** ou en **XML** (au choix, équivalents). « Nouveau plugin… » demande le format. L'ancien
`plugin.json` est encore lu pour ne pas casser les plugins existants, mais il n'est plus créé. Si plusieurs manifestes
existent dans le même dossier, l'ordre est : `plugin.yaml`, `plugin.yml`, `plugin.xml`, `plugin.json`.

### YAML

```yaml
# commentaire
format: 1
id: wool_pack
name: Laines colorées
version: "1.0"          # entre guillemets : sinon 1.0 serait un nombre
author: Vous
description: Pack de laines

icons:
  card.biomes: icons/biomes.png
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

### XML

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


### Icônes (`icons`)

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

Les icônes monochromes (noires ou blanches) prennent automatiquement la couleur du texte de leur zone ; les icônes colorées gardent leurs couleurs.

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

### Blocs, `blocksets` et `textures`

- `blocks` : chaque entrée apparaît dans le sélecteur (bouton « … » ou « + » à côté des champs bloc) avec sa texture
  (PNG, 16×16 conseillé). Les listes déroulantes montrent les 300 premiers blocs de plugins ; le sélecteur les montre tous.
- `blocksets` : génère plusieurs blocs depuis un motif (`{name}` remplacé pour chaque nom de `names`).
- `textures` : image d'aperçu pour n'importe quelle référence existante (par exemple `default:stone`).
- Si un bloc appartient à un autre mod (`wool:blue`), **ce mod est ajouté automatiquement aux dépendances** du mod créé
  (`mod.conf`). Il doit exister dans le jeu utilisé pour tester (Minetest Game contient `wool`).
- Le champ de saisie à côté du sélecteur accepte aussi n'importe quelle référence `mod:nom` écrite à la main.

### Modèles prédéfinis (`presets`)

`tab` : `nodes models items foods tools crafts ores biomes decos entities cmds`, ou l'`id` d'une fonctionnalité du plugin.
`values` utilise les clés des champs de la page (ex. biomes : `node_top depth_top node_filler depth_filler node_stone ymin ymax heat humid`).
Le menu **Modèles ▾** apparaît sous la liste de la page concernée.

### Extraits Lua (`snippets`)

Cochés dans Plugins → « Extraits Lua du projet » ; le code est ajouté à la fin de `init.lua`. Avec `"template": true`, le fichier passe
par le moteur de gabarits ci-dessous (variable `modname`).

### Fonctionnalités (`features`)

Ajoute une carte sur l'accueil et une page d'édition. Chaque élément créé est transmis au gabarit, qui produit du Lua.

Champs (`fields`) : `key` (a-z0-9_), `label`, `type`, et selon le type `default`, `min`, `max`, `options`, `filter`, `hint`.

| type | valeur |
|---|---|
| `text` | texte |
| `int`, `double` | nombre (`min`, `max`) |
| `combo` | choix (`options`) |
| `check` | case à cocher |
| `multi` | texte multiligne |
| `file` | fichier : un PNG est copié dans `textures/`, un autre fichier dans `models/` ; la valeur est le nom du fichier copié |
| `ref` | référence de bloc avec sélecteur |
| `reflist` | liste de références séparées par des virgules |

Un champ `name` (nom technique) est ajouté automatiquement s'il manque.

### Gabarits

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

## Scripts (Python et autres)

Un plugin peut contenir des **scripts** qui génèrent du code à partir des paramètres d'une fonctionnalité, ou qui agissent
sur le projet. Langages : Python (`.py`, interpréteur `python3`), shell (`.sh`), Node (`.js`), Ruby (`.rb`), Lua (`.lua`,
interpréteur `lua`), Perl (`.pl`). L'interpréteur doit être installé sur votre machine.

> **Sécurité.** Un script est un vrai programme lancé avec **vos droits** (fichiers, réseau…) : LuantiCreator ne l'isole pas.
> Les scripts d'un plugin ne tournent que si vous cochez « **Autoriser les scripts de ce plugin** » (gestionnaire de plugins, liste des plugins installés).
> L'autorisation est liée au contenu du plugin : si ses fichiers changent (mise à jour, remplacement), elle disparaît.
> Quand **vous** modifiez votre propre plugin avec l'éditeur intégré, elle est conservée. Durée maximale : 30 s par script.

### Où déclarer un script

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

### La bibliothèque `luanti_creator` (Python)

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
`key` des champs pour une fonctionnalité de plugin).

### Événement `startup`

```yaml
scripts:
  - id: theme
    name: Mon thème
    file: scripts/theme.py
    event: startup        # au lancement de LuantiCreator et à chaque rechargement des plugins
```
Il sert aux modifications de l'interface (ci-dessous). Il n'a pas de projet à lire (`lc.project` est vide) et ne produit pas de Lua.

### Modifier LuantiCreator (`luanti_creator_core`)

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

### Autres langages

Sans la bibliothèque, le protocole est simple : variables d'environnement `LC_CONTEXT` (fichier JSON du contexte : `event`,
`app`, `mod`, `plugin`, `feature`, `items`, `project`), `LC_RESULT` (fichier JSON de résultat à écrire :
`{"lua": "...", "log": [], "warnings": [], "errors": [], "add_items": {"items": [{...}]}}`), `LC_OUT_DIR` (fichiers à ajouter au mod),
`LC_PLUGIN_DIR`. **Sans fichier résultat, la sortie standard du script est prise comme code Lua** (ex : un script `.sh` qui fait `echo`).
Un code de sortie non nul est signalé comme une erreur.

Exemple complet : `examples/script_demo.zip` (fonctionnalité « Dés » générée par Python, script manuel « Créer des gemmes »,
script de fin de génération).

## Éditeur de formspec

La carte **Formspec** de l'accueil permet de dessiner des interfaces de jeu par glisser-déposer ; elles sont enregistrées dans le projet
(clé `formspecs`) et leur code Lua est ajouté à `init.lua`. Les scripts reçoivent aussi ces formulaires : `lc.project["formspecs"]`
(liste de `{name, w, h, bg, command, on_fields, elems: [{type, x, y, w, h, name, text, extra, flag}]}`).

## Données du projet

Les éléments des fonctionnalités et les extraits cochés sont enregistrés dans le projet (`.lcproj`). Si le plugin est
désinstallé, ses données sont conservées et réapparaissent s'il est réinstallé.

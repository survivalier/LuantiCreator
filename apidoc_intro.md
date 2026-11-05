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
- Pour partager : **Exporter en .zip…** ; pour installer un plugin reçu : **Installer depuis un ZIP…**.

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

"""luanti_creator_core : modifier des parties EXISTANTES de LuantiCreator depuis un plugin (usage avancé).

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
"""
import luanti_creator as _lc

API_VERSION = 1

WARNING = ("Ce plugin utilise luanti_creator_core : le comportement de LuantiCreator peut être altéré "
           "à cause de lui.")

allowed = _lc.core_allowed   # True si l'utilisateur a autorisé ce plugin à modifier LuantiCreator
_lc._core_used = True        # LuantiCreator le sait dès l'import (avertissement, contrôle d'autorisation)

_MAX_QSS = 64 * 1024


def _patch(op, **kw):
    kw["op"] = op
    _lc._core_patches.append(kw)


def _text(v, name, maxlen=2000):
    if not isinstance(v, str):
        raise TypeError("%s doit être une chaîne" % name)
    if len(v) > maxlen:
        raise ValueError("%s est trop long (max %d caractères)" % (name, maxlen))
    return v


# ------------------------------------------------------------------ interface
def set_window_title(text):
    """Change le titre de la fenêtre principale."""
    _patch("window_title", text=_text(text, "text", 200))


def set_stylesheet(qss):
    """Ajoute une feuille de style Qt (QSS) à toute l'application (64 Ko max), appliquée APRÈS celle de LuantiCreator.
    Elle remplace la feuille « principale » précédente du plugin (voir add_stylesheet pour en avoir plusieurs)."""
    _patch("stylesheet", qss=_text(qss, "qss", _MAX_QSS), name="")


def add_stylesheet(name, qss):
    """Ajoute une feuille de style NOMMÉE (64 Ko max). Un même nom remplace la feuille précédente ; des noms
    différents se cumulent. Pratique pour découper un style en plusieurs morceaux (boutons, listes, onglets...)."""
    _patch("stylesheet", qss=_text(qss, "qss", _MAX_QSS), name=_text(name, "name", 60) or "")


THEME_COLORS = ("bg", "surface", "surface2", "border", "text", "muted", "accent", "accentHover", "accentText",
                "hover", "scroll", "danger", "disabled", "placeholder")


def set_theme_colors(**colors):
    """Change les couleurs du thème (sombre ET clair), au format "#rrggbb". Noms possibles :
      bg (fond de fenêtre), surface (champs, listes, cartes), surface2 (boutons, survol des cartes), border (bordures),
      text (texte), muted (texte secondaire), accent (couleur d'accent), accentHover, accentText (texte sur l'accent),
      hover (survol des éléments), scroll (barres de défilement), danger, disabled, placeholder.
    Ces couleurs sont utilisées dans les feuilles de style : `core.add_stylesheet` peut aussi les reprendre à la main."""
    if not colors:
        raise ValueError("set_theme_colors : aucune couleur donnée")
    out = {}
    for k, v in colors.items():
        if k not in THEME_COLORS:
            raise ValueError("couleur inconnue « %s » (possibles : %s)" % (k, ", ".join(THEME_COLORS)))
        v = _text(v, k, 7)
        if len(v) != 7 or v[0] != "#" or any(ch not in "0123456789abcdefABCDEF" for ch in v[1:]):
            raise ValueError("%s : couleur invalide « %s » (format attendu : #rrggbb)" % (k, v))
        out[k] = v
    _patch("theme", colors=out)


def set_font(family=None, size=None):
    """Change la police de l'application (`family` : nom de police, `size` : taille en points, 6 à 32)."""
    p = {}
    if family is not None:
        p["family"] = _text(family, "family", 100)
    if size is not None:
        size = float(size)
        if not 6 <= size <= 32:
            raise ValueError("size doit être entre 6 et 32")
        p["size"] = size
    if not p:
        raise ValueError("set_font : rien à changer")
    _patch("font", **p)


def set_card(card_id, title=None, description=None, emoji=None, hidden=None):
    """Modifie une carte de l'accueil (identifiants : project nodes models items foods tools crafts ores biomes decos
    entities cmds pixel formspec lua server folder export save open plugins, ou plugin.<plugin>.<fonction>).
    Seuls les paramètres donnés sont changés."""
    p = {"card": _text(card_id, "card_id", 100)}
    if title is not None:
        p["title"] = _text(title, "title", 100)
    if description is not None:
        p["description"] = _text(description, "description", 300)
    if emoji is not None:
        p["emoji"] = _text(emoji, "emoji", 16)
    if hidden is not None:
        p["hidden"] = bool(hidden)
    _patch("card", **p)


def hide_card(card_id):
    """Masque une carte de l'accueil."""
    set_card(card_id, hidden=True)


def show_card(card_id):
    """Réaffiche une carte masquée."""
    set_card(card_id, hidden=False)


def set_icon(key, rel_file, icon_only=False):
    """Fournit l'icône d'une clé (voir PLUGINS.md : card.*, btn.*, cat.*...), en PRIORITÉ sur les icônes des
    plugins. `rel_file` : image du plugin (chemin relatif à son dossier)."""
    rel = _lc._check_rel(rel_file)
    _patch("icon", key=_text(key, "key", 200), file=rel, icon_only=bool(icon_only))


# ------------------------------------------------------------------ code Lua généré
def lua_prepend(code):
    """Ajoute du Lua au DÉBUT de l'init.lua généré (événements generate / feature)."""
    _patch("lua_prepend", code=_text(code, "code", 1000000))


def lua_append(code):
    """Ajoute du Lua à la FIN de l'init.lua généré (événements generate / feature)."""
    _patch("lua_append", code=_text(code, "code", 1000000))


def lua_replace(old, new, count=0):
    """Remplace `old` par `new` dans l'init.lua généré (count = 0 : toutes les occurrences)."""
    if not old:
        raise ValueError("lua_replace : `old` est vide")
    _patch("lua_replace", old=_text(old, "old", 100000), new=_text(new, "new", 1000000), count=int(count))

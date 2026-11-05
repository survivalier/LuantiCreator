#pragma once
// GÉNÉRÉ par gen_header.py depuis luanti_creator.py et luanti_creator_core.py : ne modifiez que les .py puis relancez  python3 gen_header.py
// Bibliothèques Python fournies aux scripts des plugins (écrites dans un dossier temporaire à chaque exécution).
namespace lcsdk {
inline const char *python()
{
    return R"PYSDK("""luanti_creator : bibliothèque pour les scripts Python des plugins LuantiCreator.

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
"""
import atexit
import json
import os
import re
import shutil
import sys

API_VERSION = 1

# ------------------------------------------------------------------ contexte
def _load():
    path = os.environ.get("LC_CONTEXT")
    if path and os.path.isfile(path):
        with open(path, "r", encoding="utf-8") as f:
            return json.load(f)
    return {}

context = _load()                          # le contexte brut (dict)
event = context.get("event", "manual")     # "feature", "generate" ou "manual"
app = context.get("app", {})               # {"name", "version"}
mod = context.get("mod", {})               # {"id", "title", "description", "dir"}
plugin = context.get("plugin", {})         # {"id", "name", "version", "dir"}
feature = context.get("feature", {})       # {"id", "key", "title", "prefix"} (événement "feature")
items = list(context.get("items", []))     # éléments de la fonctionnalité ; chaque dict a aussi _ident et _ref
project = context.get("project", {})       # toutes les pages du projet : {"nodes": [...], "items": [...], ...}
mod_name = mod.get("id", "") or "mon_mod"  # identifiant technique du mod
core_allowed = bool(context.get("core_allowed", False))  # True si l'utilisateur a autorisé ce plugin à modifier LuantiCreator

_out_dir = os.environ.get("LC_OUT_DIR", "")
_result_path = os.environ.get("LC_RESULT", "")
_lua = []
_log = []
_warnings = []
_errors = []
_add_items = {}
_files = []
_core_patches = []   # modifications demandées via luanti_creator_core (appliquées seulement si autorisées)
_core_used = False   # passe à True dès que luanti_creator_core est importé


# ------------------------------------------------------------------ génération de Lua
def emit(code="", end="\n"):
    """Ajoute du code Lua au mod (init.lua)."""
    _lua.append(str(code) + end)


def lua_str(s):
    """Chaîne Python -> littéral de chaîne Lua entre guillemets."""
    s = str(s)
    out = []
    for ch in s:
        o = ord(ch)
        if ch == "\\":
            out.append("\\\\")
        elif ch == '"':
            out.append('\\"')
        elif ch == "\n":
            out.append("\\n")
        elif ch == "\r":
            out.append("\\r")
        elif ch == "\t":
            out.append("\\t")
        elif o < 32 or o == 127:
            out.append("\\%03d" % o)
        else:
            out.append(ch)
    return '"' + "".join(out) + '"'


_IDENT = re.compile(r"^[A-Za-z_][A-Za-z0-9_]*$")
_LUA_KEYWORDS = {"and", "break", "do", "else", "elseif", "end", "false", "for", "function", "goto", "if", "in",
                 "local", "nil", "not", "or", "repeat", "return", "then", "true", "until", "while"}


def lua_value(v, indent=0):
    """Valeur Python -> littéral Lua (None, bool, nombres, str, list -> table, dict -> table)."""
    pad = "    " * (indent + 1)
    end = "    " * indent
    if v is None:
        return "nil"
    if v is True:
        return "true"
    if v is False:
        return "false"
    if isinstance(v, int):
        return str(v)
    if isinstance(v, float):
        return repr(int(v)) if v == int(v) and abs(v) < 1e15 else repr(v)
    if isinstance(v, str):
        return lua_str(v)
    if isinstance(v, (list, tuple)):
        if not v:
            return "{}"
        return "{\n" + "".join(pad + lua_value(x, indent + 1) + ",\n" for x in v) + end + "}"
    if isinstance(v, dict):
        if not v:
            return "{}"
        parts = []
        for k, x in v.items():
            if isinstance(k, str) and _IDENT.match(k) and k not in _LUA_KEYWORDS:
                key = k
            else:
                key = "[" + lua_value(k) + "]"
            parts.append(pad + key + " = " + lua_value(x, indent + 1) + ",\n")
        return "{\n" + "".join(parts) + end + "}"
    raise TypeError("valeur non convertible en Lua : %r" % (v,))


def ident(s):
    """Texte -> identifiant technique (a-z, 0-9, _), comme dans LuantiCreator."""
    s = str(s).strip().lower()
    s = re.sub(r"[^a-z0-9_]+", "_", s).strip("_")
    return s


def render(template, **values):
    """Remplace {{cle}} dans un texte (substitution simple, pas de logique)."""
    def sub(m):
        k = m.group(1).strip()
        return str(values[k]) if k in values else m.group(0)
    return re.sub(r"\{\{\s*([A-Za-z0-9_]+)\s*\}\}", sub, template)


# ------------------------------------------------------------------ messages
def log(msg):
    """Message d'information (console de LuantiCreator)."""
    _log.append(str(msg))


def warn(msg):
    """Avertissement affiché avec ceux de la génération."""
    _warnings.append(str(msg))


def error(msg):
    """Erreur : la génération continue mais le script est signalé en échec."""
    _errors.append(str(msg))


def fail(msg):
    """Signale une erreur et arrête le script."""
    error(msg)
    sys.exit(1)


# ------------------------------------------------------------------ fichiers
_SAFE_REL = re.compile(r"^[A-Za-z0-9_\-./]+$")


def _check_rel(rel):
    rel = str(rel).replace("\\", "/")
    if not rel or rel.startswith("/") or ".." in rel.split("/") or not _SAFE_REL.match(rel):
        raise ValueError("chemin non autorisé : %r (lettres, chiffres, _ - . / ; pas de '..')" % rel)
    return rel


def add_file(rel, data):
    """Écrit un fichier dans le dossier du mod (ex : "textures/mon_bloc.png", "sounds/x.ogg").
    `data` : bytes ou str. Disponible quand le mod est généré (événements "feature" et "generate")."""
    rel = _check_rel(rel)
    if not _out_dir:
        warn("add_file(%s) ignoré : pas de dossier de sortie (script lancé hors de LuantiCreator ?)" % rel)
        return
    dst = os.path.join(_out_dir, *rel.split("/"))
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    with open(dst, "wb") as f:
        f.write(data.encode("utf-8") if isinstance(data, str) else data)
    if rel not in _files:
        _files.append(rel)


def copy_file(src, rel):
    """Copie un fichier existant dans le mod (ex : copy_file(item["fichier"], "models/x.obj"))."""
    with open(src, "rb") as f:
        add_file(rel, f.read())


def asset(rel):
    """Chemin absolu d'un fichier du plugin (images, données...)."""
    rel = _check_rel(rel)
    return os.path.join(plugin.get("dir", "."), *rel.split("/"))


def read_asset(rel, binary=False):
    """Contenu d'un fichier du plugin."""
    with open(asset(rel), "rb" if binary else "r", **({} if binary else {"encoding": "utf-8"})) as f:
        return f.read()


# ------------------------------------------------------------------ projet
def names(page):
    """Noms (champ « name ») des éléments d'une page du projet."""
    return [str(x.get("name", "")) for x in project.get(page, [])]


def find(page, name):
    """Premier élément de la page dont le nom (ou l'identifiant) vaut `name`, sinon None."""
    for x in project.get(page, []):
        if x.get("name") == name or x.get("_ident") == name:
            return x
    return None


def add_item(page, values):
    """Ajoute un élément à une page du projet ("nodes", "items", "biomes", "plugin.<plugin>.<fonction>"...).
    Il apparaît dans LuantiCreator à la fin du script. Surtout utile pour les scripts « manuels »."""
    if not isinstance(values, dict):
        raise TypeError("add_item attend un dict")
    _add_items.setdefault(page, []).append(values)


# ------------------------------------------------------------------ transmission du résultat
def _flush():
    if not _result_path:
        return
    result = {
        "api": API_VERSION,
        "lua": "".join(_lua),
        "log": _log,
        "warnings": _warnings,
        "errors": _errors,
        "add_items": _add_items,
        "files": _files,
        "core_used": _core_used,
        "core": _core_patches,
    }
    tmp = _result_path + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump(result, f, ensure_ascii=False)
    os.replace(tmp, _result_path)


def _hook(tp, val, tb):
    import traceback
    _errors.append("".join(traceback.format_exception_only(tp, val)).strip())
    traceback.print_exception(tp, val, tb)


sys.excepthook = _hook
atexit.register(_flush)
)PYSDK";
}
inline const char *pythonCore()
{
    return R"PYSDK("""luanti_creator_core : modifier des parties EXISTANTES de LuantiCreator depuis un plugin (usage avancé).

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
)PYSDK";
}
} // namespace lcsdk

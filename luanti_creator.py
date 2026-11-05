"""luanti_creator : bibliothèque pour les scripts Python des plugins LuantiCreator.

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

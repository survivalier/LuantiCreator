#!/usr/bin/env python3
"""Génère apidoc.h : la documentation complète de LuantiCreator affichée dans l'application
(guide_utilisateur.md + apidoc_intro.md + PLUGINS.md + référence extraite des docstrings de luanti_creator.py et luanti_creator_core.py).
Appelé par gen_header.py ; relancez  python3 gen_header.py  après avoir modifié l'un de ces fichiers."""
import ast
import os
import re

here = os.path.dirname(os.path.abspath(__file__))


def read(name):
    return open(os.path.join(here, name), encoding="utf-8").read()


def demote(md):
    """PLUGINS.md : un niveau de titre en moins (hors blocs de code)."""
    out, fence = [], False
    for line in md.split("\n"):
        if line.lstrip().startswith("```"):
            fence = not fence
        elif not fence and re.match(r"#{1,5} ", line):
            line = "#" + line
        out.append(line)
    return "\n".join(out)


def hard_breaks(doc):
    lines = doc.split("\n")
    return "  \n".join(l.rstrip() for l in lines)


def signature(fn):
    a = fn.args
    parts = []
    pos = a.posonlyargs + a.args
    defaults = [None] * (len(pos) - len(a.defaults)) + list(a.defaults)
    for arg, d in zip(pos, defaults):
        parts.append(arg.arg if d is None else "%s=%s" % (arg.arg, ast.unparse(d)))
    if a.vararg:
        parts.append("*" + a.vararg.arg)
    for arg, d in zip(a.kwonlyargs, a.kw_defaults):
        parts.append(arg.arg if d is None else "%s=%s" % (arg.arg, ast.unparse(d)))
    if a.kwarg:
        parts.append("**" + a.kwarg.arg)
    return ", ".join(parts)


def reference(pyfile, alias, title, intro):
    tree = ast.parse(read(pyfile))
    mod_doc = ast.get_docstring(tree)
    out = ["# " + title, "", intro, ""]
    if mod_doc:
        out += ["## Présentation du module", "", "```text", mod_doc, "```", ""]
    out += ["## Fonctions", ""]
    for n in tree.body:
        if isinstance(n, ast.FunctionDef) and not n.name.startswith("_"):
            out += ["### `%s.%s(%s)`" % (alias, n.name, signature(n)), "", hard_breaks(ast.get_docstring(n) or "(pas de description)"), ""]
    return "\n".join(out)


def build():
    intro = read("apidoc_intro.md").rstrip() + "\n"
    manifest = "# Référence : manifeste, icônes, scripts\n\n" + demote(read("PLUGINS.md")).split("\n", 1)[1]
    lc_intro = ("Bibliothèque Python fournie avec LuantiCreator (rien à installer) : `import luanti_creator as lc`. "
                "Variables du module : `lc.context` (contexte brut), `lc.event` (`feature`, `generate`, `manual` ou `startup`), `lc.app`, `lc.mod`, "
                "`lc.mod_name`, `lc.plugin`, `lc.feature`, `lc.items`, `lc.project`, `lc.core_allowed`, `lc.API_VERSION`.")
    core_intro = ("Module `import luanti_creator_core as core` : modifie des parties de LuantiCreator. Exige `core: true` dans le manifeste "
                  "et l'autorisation « Autoriser ce plugin à modifier LuantiCreator ». `core.allowed` vaut `True` si le plugin est autorisé. "
                  "Les fonctions d'interface fonctionnent dans les événements `startup` et `manual` ; les fonctions `lua_*` dans `generate` et `feature`.")
    guide = read("guide_utilisateur.md").rstrip() + "\n"
    parts = [guide, intro, manifest,
             reference("luanti_creator.py", "lc", "Référence de l'API Python : luanti_creator", lc_intro),
             reference("luanti_creator_core.py", "core", "Référence de l'API Python : luanti_creator_core", core_intro)]
    return "\n\n".join(p.rstrip() for p in parts) + "\n"


def write():
    md = build()
    assert ')APIDOC"' not in md
    chunks = [md[i:i + 6000] for i in range(0, len(md), 6000)]
    out = ('#pragma once\n// GÉNÉRÉ par gen_apidoc.py (via gen_header.py) : ne modifiez pas à la main.\n'
           '#include <QByteArray>\n#include <QString>\n\n'
           'namespace lcapidoc {\ninline QString markdown()\n{\n    QByteArray r;\n' +
           "\n".join('    r.append(R"APIDOC(%s)APIDOC");' % c for c in chunks) +
           '\n    return QString::fromUtf8(r);\n}\n} // namespace lcapidoc\n')
    open(os.path.join(here, "apidoc.h"), "w", encoding="utf-8").write(out)
    print("apidoc.h régénéré (%d caractères)" % len(md))


if __name__ == "__main__":
    write()

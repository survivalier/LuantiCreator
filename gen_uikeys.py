#!/usr/bin/env python3
"""Régénère uikeys.h : les clés d'icônes de plugin RÉELLEMENT utilisées dans le code de l'application.

Le script lit tous les .h / .cpp et relève les appels UiHooks::bind(), bindWindow(), bindBox() et bindRow() ;
pour chaque appel il ajoute la clé complète et toutes ses clés englobantes (voir UiHooks::candidates).
Le validateur de plugins (plugins.h) et l'éditeur d'icônes (pluginpage.h) s'en servent : une clé qui existe dans le code
n'est donc jamais signalée « inconnue ». Relancez  python3 gen_uikeys.py  après avoir ajouté un bouton / une action /
une fenêtre liés à UiHooks (build.sh le fait automatiquement quand python3 est disponible)."""
import os, re, glob

here = os.path.dirname(os.path.abspath(__file__))
STR = re.compile(r'^"((?:[^"\\]|\\.)*)"$')

def split_args(s):
    """Découpe une liste d'arguments C++ au niveau supérieur (parenthèses, accolades, crochets, chaînes)."""
    args, cur, depth, i, instr = [], "", 0, 0, False
    while i < len(s):
        c = s[i]
        if instr:
            cur += c
            if c == "\\": cur += s[i + 1]; i += 1
            elif c == '"': instr = False
        elif c == '"': instr = True; cur += c
        elif c in "([{": depth += 1; cur += c
        elif c in ")]}": depth -= 1; cur += c
        elif c == "," and depth == 0: args.append(cur.strip()); cur = ""
        else: cur += c
        i += 1
    if cur.strip(): args.append(cur.strip())
    return args

def call_args(src, start):
    """Texte entre la parenthèse ouvrante située à `start` et sa parenthèse fermante."""
    depth, i, instr = 0, start, False
    while i < len(src):
        c = src[i]
        if instr:
            if c == "\\": i += 1
            elif c == '"': instr = False
        elif c == '"': instr = True
        elif c == "(": depth += 1
        elif c == ")":
            depth -= 1
            if depth == 0: return src[start + 1:i]
        i += 1
    return ""

def lit(a):
    m = STR.match(a.strip()) if a is not None else None
    return m.group(1) if m else None

def candidates(scope, key):
    r, s = [], scope or ""
    while s:
        r.append(s + "." + key)
        s = s.rsplit(".", 1)[0] if "." in s else ""
    r.append(key)
    return r

keys = set()
for path in sorted(glob.glob(os.path.join(here, "*.h")) + glob.glob(os.path.join(here, "*.cpp"))):
    if os.path.basename(path) in ("uihooks.h", "uikeys.h", "apidoc.h", "lcsdk.h"): continue
    src = open(path, encoding="utf-8").read()
    # clés données autrement qu'en argument direct (assistant addAct(..., "menu.save"), tableaux {"tab.projects", ...})
    if os.path.basename(path) != "pluginpage.h":  # (pluginpage.h contient la liste manuelle des clés : on ne s'y fie pas)
        for k in re.findall(r'"((?:menu|tab|window)\.[a-z0-9_]+(?:\.[a-z0-9_]+)*)"', src):
            keys.add(k)
    for m in re.finditer(r'UiHooks::(bind|bindWindow|bindBox|bindRow)\s*\(', src):
        kind = m.group(1)
        args = split_args(call_args(src, m.end() - 1))
        if kind == "bind" and len(args) >= 2:
            k = lit(args[1]); sc = lit(args[2]) if len(args) > 2 else ""
            if k: keys.update(candidates(sc, k))
        elif kind == "bindWindow" and len(args) >= 2:
            n = lit(args[1])
            if n: keys.add("window." + n)
        elif kind == "bindBox" and len(args) >= 2:
            n = lit(args[1])
            if n:
                for b in ("btn.ok", "btn.cancel", "btn.accept", "btn.other"):
                    keys.update(candidates("dialog." + n, b))
        elif kind == "bindRow" and len(args) >= 2:
            sc = lit(args[2]) if len(args) > 2 else ""
            for k in re.findall(r'"((?:btn|menu)\.[\w.]+)"', args[1]):
                keys.update(candidates(sc, k))

out = ['#pragma once',
       '// GÉNÉRÉ par gen_uikeys.py à partir des appels UiHooks::bind* du code : ne modifiez pas à la main, relancez  python3 gen_uikeys.py',
       '// Clés d\'icônes de plugin qui existent réellement dans l\'application (clés complètes et clés englobantes).',
       '#include <cstddef>', '',
       'namespace lckeys {',
       'inline const char *const *codeIconKeys(std::size_t *count)',
       '{', '    static const char *const keys[] = {']
out += ['        "%s",' % k for k in sorted(keys)]
out += ['    };', '    *count = sizeof(keys) / sizeof(keys[0]);', '    return keys;', '}', '} // namespace lckeys', '']
open(os.path.join(here, "uikeys.h"), "w", encoding="utf-8").write("\n".join(out))
print("uikeys.h régénéré :", len(keys), "clés")

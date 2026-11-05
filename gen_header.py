#!/usr/bin/env python3
"""Régénère lcsdk.h (copie de luanti_creator.py et luanti_creator_core.py embarquée dans l'exécutable)."""
import os
here = os.path.dirname(os.path.abspath(__file__))

def read(name):
    s = open(os.path.join(here, name), encoding="utf-8").read()
    assert ')PYSDK"' not in s
    return s

out = ('#pragma once\n'
       '// GÉNÉRÉ par gen_header.py depuis luanti_creator.py et luanti_creator_core.py : ne modifiez que les .py '
       'puis relancez  python3 gen_header.py\n'
       '// Bibliothèques Python fournies aux scripts des plugins (écrites dans un dossier temporaire à chaque exécution).\n'
       'namespace lcsdk {\n'
       'inline const char *python()\n{\n    return R"PYSDK(' + read("luanti_creator.py") + ')PYSDK";\n}\n'
       'inline const char *pythonCore()\n{\n    return R"PYSDK(' + read("luanti_creator_core.py") + ')PYSDK";\n}\n'
       '} // namespace lcsdk\n')
open(os.path.join(here, "lcsdk.h"), "w", encoding="utf-8").write(out)
print("lcsdk.h régénéré")

import gen_apidoc
gen_apidoc.write()

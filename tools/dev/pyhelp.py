"""Prints what the editor's Python knows about a name: run_script("dev/pyhelp.py", ["MergeStaticMeshActorsOptions"])."""
import sys

import unreal

for name in sys.argv[1:]:
    thing = getattr(unreal, name, None)
    if thing is None:
        print(name, ": not in unreal;", [n for n in dir(unreal) if name.lower()[:8] in n.lower()][:30])
        continue
    print(name, ":", [n for n in dir(thing) if not n.startswith("_")][:80])
    doc = (thing.__doc__ or "")
    print(doc[:1800])

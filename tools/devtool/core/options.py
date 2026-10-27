"""Parses option(...) declarations from CompileOptions.cmake."""
import os
import re

_OPT = re.compile(r'^\s*option\s*\(\s*(\w+)\s+"((?:[^"\\]|\\.)*)"\s*([^)]*)\)', re.MULTILINE)


def load(repo_root):
    path = os.path.join(repo_root, "CompileOptions.cmake")
    with open(path, "r", encoding="utf-8") as f:
        text = f.read()
    result = []
    seen = set()
    for m in _OPT.finditer(text):
        name, desc, default = m.group(1), m.group(2), m.group(3).strip()
        if name in seen:
            continue
        seen.add(name)
        if default.upper() in ("ON", "TRUE", "1", "YES"):
            d = "ON"
        elif default.upper() in ("OFF", "FALSE", "0", "NO", ""):
            d = "OFF"
        else:
            d = "auto"  # computed default e.g. ${VKE_..._DEFAULT}
        result.append({"name": name, "description": desc, "default": d})
    return result

"""Parses samples/CMakeLists.txt add_subdirectory entries. Sample target name equals its folder name."""
import os
import re

_SUB = re.compile(r"^\s*add_subdirectory\s*\(\s*([\w\-]+)(\s+EXCLUDE_FROM_ALL)?\s*\)", re.MULTILINE)


def load(repo_root):
    path = os.path.join(repo_root, "samples", "CMakeLists.txt")
    if not os.path.isfile(path):
        return []
    with open(path, "r", encoding="utf-8") as f:
        text = f.read()
    return [{"name": m.group(1), "target": m.group(1).replace(" ", "_"), "excludeFromAll": bool(m.group(2))}
            for m in _SUB.finditer(text)]

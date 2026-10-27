"""UI state cache stored in out/devtool/ui-state.json, validated against current repo schema."""
import json
import os

FILE = "ui-state.json"


def _filter(state, schema):
    s = dict(state or {})
    s["presets"] = [p for p in s.get("presets", []) if p in schema["presets"]]
    s["configs"] = [c for c in s.get("configs", []) if c in schema["configs"]]
    s["samples"] = [x for x in s.get("samples", []) if x in schema["samples"]]
    s["overrides"] = {k: v for k, v in (s.get("overrides") or {}).items()
                      if k in schema["options"] and v in ("ON", "OFF")}
    s["extra"] = [e for e in s.get("extra", []) if isinstance(e, dict) and e.get("name")]
    return s


def load(out, fingerprint, schema):
    try:
        with open(os.path.join(out, FILE), "r", encoding="utf-8") as f:
            data = json.load(f)
    except (OSError, ValueError):
        return {"state": None, "changed": False}
    changed = data.get("fingerprint") != fingerprint
    return {"state": _filter(data.get("state"), schema), "changed": changed}


def save(out, fingerprint, schema, state):
    os.makedirs(out, exist_ok=True)
    path = os.path.join(out, FILE)
    tmp = path + ".tmp"
    with open(tmp, "w", encoding="utf-8") as f:
        json.dump({"fingerprint": fingerprint, "state": _filter(state, schema)}, f, indent=2)
    os.replace(tmp, path)

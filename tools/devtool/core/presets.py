"""CMakePresets.json loader: resolves inheritance, macros and host conditions."""
import json
import os
import re

GROUPS = ("Windows", "Linux", "Android")
RHIS = ("Vulkan", "D3D12")


def _merge(base, child):
    out = dict(base)
    for k, v in child.items():
        if k in ("cacheVariables", "environment") and isinstance(v, dict):
            m = dict(out.get(k, {}))
            m.update(v)
            out[k] = m
        elif k in ("inherits", "hidden", "name"):
            continue
        else:
            out[k] = v
    return out


def _resolve(name, raw, cache, chain=()):
    if name in cache:
        return cache[name]
    if name in chain:
        raise ValueError(f"Preset inheritance cycle at {name}")
    p = raw[name]
    inh = p.get("inherits", [])
    if isinstance(inh, str):
        inh = [inh]
    result = {}
    # CMake: earlier parents take precedence over later ones
    for parent in reversed(inh):
        result = _merge(result, _resolve(parent, raw, cache, chain + (name,)))
    if "inherits" in p:
        # vendor/msvc-env lineage tracking
        lineage = set()
        for parent in inh:
            lineage.add(parent)
            lineage |= set(cache[parent].get("_lineage", []))
        result["_lineage"] = sorted(lineage)
    result = _merge(result, p)
    result["name"] = name
    result["hidden"] = p.get("hidden", False)
    cache[name] = result
    return result


def _expand(value, preset_name, source_dir):
    if not isinstance(value, str):
        return value
    value = value.replace("${sourceDir}", source_dir).replace("${presetName}", preset_name)
    value = re.sub(r"\$env\{([^}]+)\}", lambda m: os.environ.get(m.group(1), ""), value)
    return value


def _host_of(preset):
    cond = preset.get("condition") or {}
    if cond.get("type") == "equals" and cond.get("lhs") == "${hostSystemName}":
        return cond.get("rhs")
    return None


def _label(name, generator=""):
    # Windows-Ninja-Clang-x64 -> "Ninja Multi-Config, Clang"
    parts = name.split("-")
    mid = [p for p in parts[1:] if p not in ("x64", "x86", "arm64")]
    multi = "Multi-Config" in generator
    names = {"VisualStudio": "Visual Studio", "Ninja": "Ninja"}
    mid = [names[p] + (" Multi-Config" if multi else "") if p in names else p for p in mid]
    return ", ".join(mid) if mid else name


def load(repo_root, host, wsl_available):
    """host: 'Windows' or 'Linux'. Returns list of visible preset descriptors."""
    path = os.path.join(repo_root, "CMakePresets.json")
    with open(path, "r", encoding="utf-8") as f:
        data = json.load(f)
    raw = {p["name"]: p for p in data.get("configurePresets", [])}
    cache = {}
    for n in raw:
        _resolve(n, raw, cache)

    build_map = {}
    for bp in data.get("buildPresets", []):
        build_map[(bp["configurePreset"], bp.get("configuration", ""))] = bp["name"]

    src = repo_root.replace("\\", "/")
    out = []
    for name, p in cache.items():
        if p.get("hidden"):
            continue
        group = next((g for g in GROUPS if name.startswith(g + "-")), "Other")
        target_host = _host_of(p) or group
        generator = p.get("generator", "")
        is_vs = generator.startswith("Visual Studio")
        lineage = p.get("_lineage", [])
        needs_vcvars = (not is_vs) and "msvc-env" in lineage
        # Execution mode
        if target_host == host:
            mode, available, reason = "native", True, ""
        elif host == "Windows" and target_host == "Linux":
            mode = "wsl"
            available = wsl_available
            reason = "" if wsl_available else "WSL Ubuntu is not installed or not initialized (run Setup)."
        else:
            mode, available, reason = "none", False, f"Requires a {target_host} host."
        configs = [c for (cp, c) in build_map if cp == name]
        cache_types = p.get("cacheVariables", {}).get("CMAKE_CONFIGURATION_TYPES", "Debug;Release")
        if not configs:
            configs = cache_types.split(";")
        cvars = {k: str(v) if not isinstance(v, dict) else str(v.get("value", "")) for k, v in p.get("cacheVariables", {}).items()}
        out.append({
            "name": name,
            "group": group,
            "label": _label(name, generator),
            "cacheVariables": cvars,
            "rhis": [r for r in RHIS if cvars.get(f"VKE_COMPILE_{r.upper()}_RHI", "").upper() == "ON"],
            "defaultRhi": cvars.get("VKE_RENDER_SYSTEM", ""),
            "generator": generator,
            "isVS": is_vs,
            "needsVcvars": needs_vcvars,
            "mode": mode,
            "available": available,
            "reason": reason,
            "binaryDir": _expand(p.get("binaryDir", "${sourceDir}/out/build/${presetName}"), name, src),
            "configs": configs,
            "buildPresets": {c: build_map[(name, c)] for c in configs if (name, c) in build_map},
        })
    out.sort(key=lambda x: (GROUPS.index(x["group"]) if x["group"] in GROUPS else 99, x["name"]))
    return out


def all_configs(presets):
    seen = []
    for p in presets:
        for c in p["configs"]:
            if c not in seen:
                seen.append(c)
    return seen or ["Debug", "Release"]

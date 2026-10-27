"""Setup: manifest-driven dependency checks, approved installation and resumable progress.

Check runs in the server process (no changes). Installation runs in setup_runner.py inside a task terminal:
- Windows deps (groups 'windows' and 'android') run in ONE elevated runner (single UAC prompt).
- Linux deps run in ONE runner inside WSL (Windows host) or natively (Linux host); sudo is asked once.
The runner writes out/devtool/setup/progress-<target>.json. If a step requires a restart (VS, WSL) or a manual
gate (Ubuntu user creation) the runner stops; the next Setup run re-checks and skips everything already done.
"""
import json
import os
import re
import sys
import time

from . import env

HERE = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
MANIFEST = os.path.join(HERE, "manifest", "dependencies.json")
RUNNER = os.path.join(HERE, "setup_runner.py")
VSCONFIG = os.path.join(HERE, "vs.vsconfig")


def load_manifest():
    with open(MANIFEST, "r", encoding="utf-8") as f:
        return json.load(f)


def host_groups():
    return ["windows", "android", "linux"] if env.IS_WINDOWS else ["linux"]


def target_of(dep):
    return "linux" if dep["group"] == "linux" else "windows"


def expand(text, repo, pins, sdk_proxy=""):
    if not isinstance(text, str):
        return text
    text = text.replace("{repo}", repo).replace("{sdkProxy}", sdk_proxy)
    if "{msys2}" in text:
        text = text.replace("{msys2}", env.msys2_root() or env.MSYS2_DEFAULT)
    if "{vcpkgRoot}" in text:
        text = text.replace("{vcpkgRoot}", env.vcpkg_root() or env.VCPKG_DEFAULT_WINDOWS)
    if "{vcpkgFind}" in text:
        text = text.replace("{vcpkgFind}", env.vcpkg_shell_find())
    if "{androidSdk}" in text:
        text = text.replace("{androidSdk}", env.android_sdk_root())
    text = re.sub(r"\{pin:(\w+)\}", lambda m: pins.get(m.group(1), ""), text)
    if env.IS_WINDOWS:
        # Only %VAR% (cmd style). os.path.expandvars would also expand PowerShell/shell $variables.
        text = re.sub(r"%(\w+)%", lambda m: os.environ.get(m.group(1), m.group(0)), text)
    return text


def _ver(s):
    return tuple(int(x) for x in re.findall(r"\d+", s or "")[:4])


def _ok_version(found, minimum):
    return not minimum or (found and _ver(found) >= _ver(minimum))


def _shell(cmd, cwd, linux_target, extra_paths=None):
    """Runs a check command on the dependency's target system."""
    if linux_target and env.IS_WINDOWS:
        distro = env.wsl_status()["distroName"]
        return env.run(["wsl.exe", "-d", distro, "--cd", env.to_wsl_path(cwd), "--exec", "bash", "-lc", cmd], timeout=60)
    e = dict(os.environ)
    if extra_paths:
        e["PATH"] = os.pathsep.join([os.path.expandvars(p) for p in extra_paths] + [e.get("PATH", "")])
    if env.IS_WINDOWS:
        e["PATH"] = os.pathsep.join([e.get("PATH", ""), refreshed_path()])
        return env.run(f'cd /d "{cwd}" && {cmd}', timeout=60, env=e, shell=True)
    return env.run(["bash", "-lc", f'cd "{cwd}" && {cmd}'], timeout=60, env=e)


def refreshed_path():
    """Current machine+user PATH from registry (installs done after server start are visible)."""
    if not env.IS_WINDOWS:
        return os.environ.get("PATH", "")
    import winreg
    parts = []
    for root, key in ((winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment"),
                      (winreg.HKEY_CURRENT_USER, r"Environment")):
        try:
            with winreg.OpenKey(root, key) as k:
                parts.append(os.path.expandvars(winreg.QueryValueEx(k, "Path")[0]))
        except OSError:
            pass
    return os.pathsep.join(parts)


def check_one(dep, repo, pins, environment):
    """Returns {'status': ok|missing|outdated|blocked, 'version', 'detail'}."""
    c = dep.get("check", {})
    kind = c.get("type", "cmd")
    linux_target = dep["group"] == "linux"
    res = {"status": "missing", "version": "", "detail": ""}
    wsl = environment.get("wsl") or {}
    if linux_target and env.IS_WINDOWS and not wsl.get("userReady"):
        res.update(status="blocked", detail="Requires WSL Ubuntu with a user account.")
    elif kind == "vs":
        vs = environment.get("visualStudio")
        if vs:
            res["version"] = vs.get("version", "")
            res["status"] = "ok" if _ok_version(res["version"], c.get("minVersion")) else "outdated"
            res["detail"] = vs.get("installationPath", "")
    elif kind == "wsl":
        res["status"] = "ok" if wsl.get("distro") else "missing"
        res["detail"] = wsl.get("distroName", "")
    elif kind == "wslUser":
        if not wsl.get("distro"):
            res.update(status="blocked", detail="Install WSL + Ubuntu first.")
        elif wsl.get("userReady"):
            res["status"] = "ok"
        else:
            res["detail"] = "Open 'Ubuntu' from the Start menu, create your user, then press Continue."
    elif kind == "path":
        p = expand(c["path"], repo, pins)
        res["status"] = "ok" if os.path.exists(p) else "missing"
        res["detail"] = p
    else:
        if c.get("env") and env.IS_WINDOWS and not os.environ.get(c["env"]) and not _user_env(c["env"]):
            res["detail"] = f"{c['env']} is not set."
            return res
        cmd = expand(c.get("cmd", ""), repo, pins)
        if c.get("env") and env.IS_WINDOWS and not os.environ.get(c["env"]):
            cmd = cmd.replace(f"%{c['env']}%", _user_env(c["env"]))
        code, out = _shell(cmd, repo, linux_target, c.get("paths"))
        if code != 0:
            res["detail"] = (out.strip().splitlines() or ["not found"])[-1][:200]
            return res
        regex = c.get("regex")
        m = re.search(regex, out) if regex else None
        if regex and not m:
            res["detail"] = "Unrecognized version output."
            return res
        res["version"] = m.group(1) if m else ""
        minimum = expand(c.get("minVersion", ""), repo, pins)
        res["status"] = "ok" if _ok_version(res["version"], minimum) else "outdated"
        if res["status"] == "outdated":
            res["detail"] = f"Requires >= {minimum}"
    return res


def _user_env(name):
    if not env.IS_WINDOWS:
        return ""
    import winreg
    for root, key in ((winreg.HKEY_CURRENT_USER, r"Environment"),
                      (winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment")):
        try:
            with winreg.OpenKey(root, key) as k:
                return winreg.QueryValueEx(k, name)[0]
        except OSError:
            pass
    return ""


def check(repo, out, environment, groups=None, quick=False):
    m = load_manifest()
    groups = [g for g in (groups or host_groups()) if g in host_groups()]
    pins = m.get("pins", {})
    result = []
    for dep in m["dependencies"]:
        if dep["group"] not in groups:
            continue
        st = {"status": "checking", "version": "", "detail": ""} if quick else check_one(dep, repo, pins, environment)
        result.append({"id": dep["id"], "name": dep["name"], "group": dep["group"],
                       "elevate": dep.get("elevate", False), "requiresRestart": dep.get("requiresRestart", False),
                       "gate": dep.get("install", {}).get("type") == "gate", "dependsOn": dep.get("dependsOn", []),
                       **st})
    return {"host": env.HOST, "groups": groups, "pins": pins, "dependencies": result,
            "progress": progress(out)}


# ------------------------------------------------------------------ progress / resume
def _setup_dir(out):
    d = os.path.join(out, "setup")
    os.makedirs(d, exist_ok=True)
    return d


def progress(out):
    merged = {"steps": {}, "restartRequired": False, "gate": "", "running": False, "updated": 0}
    d = _setup_dir(out)
    for target in ("windows", "linux"):
        p = os.path.join(d, f"progress-{target}.json")
        try:
            with open(p, "r", encoding="utf-8") as f:
                data = json.load(f)
        except (OSError, ValueError):
            continue
        merged["steps"].update(data.get("steps", {}))
        merged["restartRequired"] |= bool(data.get("restartRequired"))
        merged["gate"] = merged["gate"] or data.get("gate", "")
        merged["running"] |= bool(data.get("running"))
        merged["updated"] = max(merged["updated"], data.get("updated", 0))
    return merged


def pending_resume(out):
    p = progress(out)
    return {"pending": bool(p["restartRequired"] or p["gate"]), "restartRequired": p["restartRequired"],
            "gate": p["gate"]}


def confirm_gate(out, dep_id):
    """User confirmed a manual step (e.g. Ubuntu user created). Clears the gate; next check re-verifies."""
    d = _setup_dir(out)
    for target in ("windows", "linux"):
        p = os.path.join(d, f"progress-{target}.json")
        try:
            with open(p, "r", encoding="utf-8") as f:
                data = json.load(f)
        except (OSError, ValueError):
            continue
        if data.get("gate") == dep_id or not dep_id:
            data["gate"] = ""
            data["restartRequired"] = False
            with open(p, "w", encoding="utf-8") as f:
                json.dump(data, f, indent=2)


# ------------------------------------------------------------------ run
def run(repo, out, scheduler, environment, ids, proxy_cfg):
    from . import proxy as proxy_mod
    from . import tasks

    m = load_manifest()
    known = {d["id"]: d for d in m["dependencies"] if d["group"] in host_groups()}
    ids = [i for i in ids if i in known]
    if not ids:
        return {"tasks": [], "message": "Nothing selected."}
    d = _setup_dir(out)
    # Previous restart/gate state is superseded by a fresh, approved run.
    for target in ("windows", "linux"):
        try:
            os.remove(os.path.join(d, f"progress-{target}.json"))
        except OSError:
            pass
    created = []
    stamp = time.strftime("%Y%m%d-%H%M%S")
    log_dir = os.path.join(out, "logs")
    by_target = {"windows": [i for i in ids if target_of(known[i]) == "windows"],
                 "linux": [i for i in ids if target_of(known[i]) == "linux"]}
    for target, tids in by_target.items():
        if not tids:
            continue
        wsl_mode = target == "linux" and env.IS_WINDOWS
        px = proxy_cfg
        warn = ""
        if wsl_mode:
            px, warn = proxy_mod.wsl_rewrite(proxy_cfg)
        plan = {"target": target, "ids": tids, "proxy": px, "warning": warn, "created": time.time()}
        plan_path = os.path.join(d, f"plan-{target}-{stamp}.json")
        with open(plan_path, "w", encoding="utf-8") as f:
            json.dump(plan, f, indent=2)
        if wsl_mode:
            cmd = f"python3 '{env.to_wsl_path(RUNNER)}' --plan '{env.to_wsl_path(plan_path)}'"
            mode = "wsl"
        elif env.IS_WINDOWS:
            cmd = f'"{sys.executable}" "{RUNNER}" --plan "{plan_path}"'
            needs_admin = any(known[i].get("elevate") for i in tids)
            mode = "elevated" if needs_admin else "native"
        else:
            cmd = f"python3 '{RUNNER}' --plan '{plan_path}'"
            mode = "native"
        t = tasks.Task("setup", f"Setup ({target})", [cmd], log_dir, mode=mode, cwd=repo,
                       task_env=proxy_mod.task_env(px))
        created.append(scheduler.submit(t).id)
    return {"tasks": created}

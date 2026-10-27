"""Setup runner: executes an approved setup plan inside a task terminal.

Windows: launched once elevated (single UAC prompt) for all Windows/Android dependencies.
Linux/WSL: launched in bash; asks for sudo once and keeps the credential alive.
Writes out/devtool/setup/progress-<target>.json so the UI can show progress and resume after restarts.
"""
import argparse
import json
import os
import subprocess
import sys
import tempfile
import threading
import time

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from core import env, setup  # noqa: E402
from core import proxy as proxy_mod  # noqa: E402

REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(REPO, "out", "devtool")

PS_PRELUDE = r"""
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'
function Add-ToPath([string]$Dir, [string]$Scope = 'User', [switch]$Append) {
    $cur = [Environment]::GetEnvironmentVariable('Path', $Scope)
    $items = @($cur -split ';' | Where-Object { $_ })
    if ($items -notcontains $Dir) {
        if ($Append) { $items += $Dir } else { $items = @($Dir) + $items }
        [Environment]::SetEnvironmentVariable('Path', ($items -join ';'), $Scope)
    }
    $env:Path = "$Dir;$env:Path"
}
function Get-Download([string]$Url, [string]$OutFile) {
    $p = if ($env:HTTPS_PROXY) { $env:HTTPS_PROXY } elseif ($env:HTTP_PROXY) { $env:HTTP_PROXY } else { $null }
    if ($p) { Invoke-WebRequest -Uri $Url -OutFile $OutFile -Proxy $p -ProxyUseDefaultCredentials -UseBasicParsing }
    else { Invoke-WebRequest -Uri $Url -OutFile $OutFile -UseBasicParsing }
}
"""


def log(msg):
    print(f"[setup] {msg}", flush=True)


class Progress:
    def __init__(self, target):
        os.makedirs(os.path.join(OUT, "setup"), exist_ok=True)
        self.path = os.path.join(OUT, "setup", f"progress-{target}.json")
        self.data = {"target": target, "steps": {}, "restartRequired": False, "gate": "", "running": True,
                     "updated": time.time()}
        self.save()

    def step(self, dep_id, status, detail=""):
        self.data["steps"][dep_id] = {"status": status, "detail": detail, "time": time.time()}
        self.save()

    def save(self):
        self.data["updated"] = time.time()
        tmp = self.path + ".tmp"
        with open(tmp, "w", encoding="utf-8") as f:
            json.dump(self.data, f, indent=2)
        os.replace(tmp, self.path)


def sh(argv, shell=False, extra_env=None):
    log("> " + (argv if isinstance(argv, str) else " ".join(argv)))
    e = dict(os.environ)
    if extra_env:
        e.update(extra_env)
    return subprocess.call(argv, shell=shell, env=e)


def run_ps(lines):
    with tempfile.NamedTemporaryFile("w", suffix=".ps1", delete=False, encoding="utf-8-sig") as f:
        f.write(PS_PRELUDE + "\n" + "\n".join(lines) + "\nexit 0\n")
        path = f.name
    try:
        return sh(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", path])
    finally:
        os.remove(path)


def refresh_path():
    if env.IS_WINDOWS:
        os.environ["PATH"] = setup.refreshed_path() + os.pathsep + os.environ.get("PATH", "")


# ------------------------------------------------------------------ installers
def install_windows(dep, pins, px):
    inst = dep.get("install", {})
    kind = inst.get("type")
    sdk_proxy = " ".join(proxy_mod.sdkmanager_args(px))
    code = 0
    if kind == "vs":
        code = install_vs(inst)
    elif kind == "wsl":
        code = sh(["wsl.exe", "--install", "-d", env.WSL_DISTRO, "--no-launch"])
        if code != 0:
            code = sh(["wsl.exe", "--install", "-d", env.WSL_DISTRO])
    else:
        if inst.get("winget"):
            code = sh(["winget", "install", "--id", inst["winget"], "-e", "--silent", "--scope", "machine",
                       "--accept-source-agreements", "--accept-package-agreements"])
            # winget returns non-zero "already installed / no upgrade" codes; trust the re-check instead.
            if code != 0:
                log(f"winget exit code {code}; verifying installation state...")
                code = 0
            refresh_path()
        for c in inst.get("cmd", []):
            if code == 0:
                code = sh(setup.expand(c, REPO, pins, sdk_proxy), shell=True)
        if code == 0 and inst.get("ps"):
            code = run_ps([setup.expand(c, REPO, pins, sdk_proxy) for c in inst["ps"]])
        refresh_path()
    return code


def install_vs(inst):
    vs = env.visual_studio()
    if vs and os.path.isfile(vs.get("installer", "")):
        return sh([vs["installer"], "modify", "--installPath", vs["installationPath"], "--config", setup.VSCONFIG,
                   "--passive", "--norestart"])
    return sh(["winget", "install", "--id", inst["winget"], "-e", "--accept-source-agreements",
               "--accept-package-agreements", "--override",
               f"--passive --norestart --wait --config \"{setup.VSCONFIG}\""])


class Sudo:
    def __init__(self):
        self.prefix = [] if os.geteuid() == 0 else ["sudo", "-E"]
        if self.prefix:
            log("Administrator privileges are required once for package installation.")
            if subprocess.call(["sudo", "-v"]) != 0:
                raise SystemExit("sudo authentication failed")
            threading.Thread(target=self._keepalive, daemon=True).start()

    def _keepalive(self):
        while True:
            time.sleep(60)
            subprocess.call(["sudo", "-n", "-v"])


def install_linux(dep, pins, px, pm, sudo, apt_updated):
    inst = dep.get("install", {})
    code = 0
    pkgs = inst.get(pm or "", [])
    if pkgs:
        if pm == "apt":
            if not apt_updated[0]:
                code = sh(sudo.prefix + ["apt-get"] + proxy_mod.apt_args(px) + ["update"])
                apt_updated[0] = code == 0
            if code == 0:
                code = sh(sudo.prefix + ["apt-get"] + proxy_mod.apt_args(px) + ["install", "-y"] + pkgs,
                          extra_env={"DEBIAN_FRONTEND": "noninteractive"})
        elif pm == "pacman":
            code = sh(sudo.prefix + ["pacman", "-S", "--needed", "--noconfirm"] + pkgs)
    elif not inst.get("sh"):
        log(f"No installer for package manager '{pm}'.")
        code = 1
    for c in inst.get("sh", []):
        if code == 0:
            code = sh(["bash", "-lc", f'cd "{REPO}" && {setup.expand(c, REPO, pins)}'])
    return code


# ------------------------------------------------------------------ main
def ordered(ids, known):
    out, seen = [], set()

    def visit(i):
        if i in seen or i not in known:
            return
        seen.add(i)
        for d in known[i].get("dependsOn", []):
            if d in ids:
                visit(d)
        out.append(i)
    for i in ids:
        visit(i)
    return out


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--plan", required=True)
    a = ap.parse_args()
    with open(a.plan, "r", encoding="utf-8") as f:
        plan = json.load(f)
    px = plan.get("proxy", {})
    # Elevated processes do not inherit the caller's environment; re-apply proxy from the plan.
    os.environ.update({k: v for k, v in proxy_mod.task_env(px, {}).items() if k != "WSLENV"})
    if plan.get("warning"):
        log(plan["warning"])

    m = setup.load_manifest()
    pins = m.get("pins", {})
    known = {d["id"]: d for d in m["dependencies"]}
    target = plan["target"]
    prog = Progress(target)
    environment = env.summary()
    failures = 0
    sudo = None
    pm = env.linux_distro().get("pm") if not env.IS_WINDOWS else None
    apt_updated = [False]

    for dep_id in ordered(plan["ids"], known):
        dep = known[dep_id]
        log(f"=== {dep['name']} ===")
        unmet = [d for d in dep.get("dependsOn", []) if prog.data["steps"].get(d, {}).get("status") == "failed"]
        if unmet:
            prog.step(dep_id, "skipped", f"Dependency failed: {', '.join(unmet)}")
            log("Skipped: dependency failed.")
            continue
        st = setup.check_one(dep, REPO, pins, environment)
        if st["status"] == "ok":
            prog.step(dep_id, "ok", f"Already installed {st['version']}".strip())
            log("Already satisfied, skipping.")
            continue
        if dep.get("install", {}).get("type") == "gate":
            prog.data["gate"] = dep_id
            prog.step(dep_id, "waiting", st.get("detail") or "Manual action required.")
            log("Manual action required: " + (st.get("detail") or ""))
            break
        prog.step(dep_id, "installing")
        try:
            if target == "windows":
                code = install_windows(dep, pins, px)
            else:
                sudo = sudo or Sudo()
                code = install_linux(dep, pins, px, pm, sudo, apt_updated)
        except (OSError, SystemExit) as e:
            log(f"Error: {e}")
            code = 1
        if code == 0 and dep.get("requiresRestart"):
            prog.data["restartRequired"] = True
            prog.step(dep_id, "restart", "Installed. Restart Windows, then run Setup again to continue.")
            log("A restart is required. Restart Windows and run Setup again; completed steps will be skipped.")
            break
        environment = env.summary()
        st = setup.check_one(dep, REPO, pins, environment) if code == 0 else {"status": "failed", "detail": f"exit {code}"}
        if st["status"] == "ok":
            prog.step(dep_id, "ok", f"Installed {st['version']}".strip())
        else:
            failures += 1
            prog.step(dep_id, "failed", st.get("detail") or f"exit {code}")
            log(f"FAILED: {st.get('detail')}")

    prog.data["running"] = False
    prog.save()
    log("Done." if not failures else f"{failures} step(s) failed.")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())

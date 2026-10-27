"""VKE DevTool server: local HTTP + JSON API + static web UI.

Usage: python server.py [--no-browser] [--port N] [--restart]
Binds to 127.0.0.1 on an OS-assigned free port (never takes over an occupied port). If an instance is already
running (out/devtool/server.json with live PID answering /api/ping) the browser is opened on it instead,
unless --restart is given (used by the launchers), which stops it and starts a fresh server.
"""
import argparse
import glob
import hashlib
import http.server
import json
import os
import secrets
import signal
import socketserver
import subprocess
import sys
import threading
import time
import urllib.parse
import urllib.request
import webbrowser

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)

from core import cache, env, options, presets, proxy, samples, setup, tasks  # noqa: E402

REPO = os.path.abspath(os.path.join(HERE, "..", ".."))
OUT = os.path.join(REPO, "out", "devtool")
SERVER_JSON = os.path.join(OUT, "server.json")
SESSION_JSON = os.path.join(OUT, "session.json")
WEB = os.path.join(HERE, "web")
WSL_ROOT = "$HOME/devtool/" + os.path.basename(REPO)
FINGERPRINT_FILES = ["CMakePresets.json", "CompileOptions.cmake", os.path.join("samples", "CMakeLists.txt")]


class App:
    def __init__(self):
        os.makedirs(OUT, exist_ok=True)
        self.scheduler = tasks.Scheduler()
        self.lock = threading.Lock()
        self._env = None
        self.reload()

    # ------------------------------------------------------------ model
    def environment(self, refresh=False):
        if self._env is None or refresh:
            self._env = env.summary()
        return self._env

    def wsl_ready(self):
        w = self.environment().get("wsl") or {}
        return bool(w.get("userReady"))

    def reload(self):
        with self.lock:
            e = self.environment(refresh=True)
            self.presets = presets.load(REPO, env.HOST, self.wsl_ready())
            vs = e.get("visualStudio")
            for p in self.presets:
                if p["available"] and p["mode"] == "native" and (p["isVS"] or p["needsVcvars"]) and not vs:
                    p["available"] = False
                    p["reason"] = "Visual Studio not found (run Setup)."
            self.options = options.load(REPO)
            preset_vars = {}
            for p in self.presets:
                for k, v in p["cacheVariables"].items():
                    preset_vars.setdefault(k, set()).add(v.upper())
            for o in self.options:
                if o["name"] in preset_vars:
                    o["default"] = "auto"
                    o["presetValues"] = sorted(preset_vars[o["name"]])
            self.samples = samples.load(REPO)
            self.configs = presets.all_configs(self.presets)
            self.fingerprint = self._fingerprint()

    def _fingerprint(self):
        h = hashlib.sha256()
        for rel in FINGERPRINT_FILES:
            try:
                with open(os.path.join(REPO, rel), "rb") as f:
                    h.update(f.read())
            except OSError:
                h.update(b"missing:" + rel.encode())
        return h.hexdigest()

    def schema(self):
        """Names the cached UI state is validated against."""
        return {
            "presets": sorted(p["name"] for p in self.presets),
            "options": sorted(o["name"] for o in self.options),
            "samples": sorted(s["name"] for s in self.samples),
            "configs": list(self.configs),
        }

    def preset(self, name):
        return next((p for p in self.presets if p["name"] == name), None)

    def gen_state_path(self, p):
        return os.path.join(p["binaryDir"], "devtool-state.json")

    def gen_state(self, p):
        try:
            with open(self.gen_state_path(p), "r", encoding="utf-8") as f:
                st = json.load(f)
        except (OSError, ValueError):
            return {"generated": False}
        st["generated"] = bool(st.get("ok")) and (p["mode"] == "wsl" or
                                                  os.path.isfile(os.path.join(p["binaryDir"], "CMakeCache.txt")))
        st["stale"] = st.get("fingerprint") != self.fingerprint
        st["solution"] = self.solution(p) if p["isVS"] else None
        return st

    def solution(self, p):
        for pat in ("*.slnx", "*.sln"):
            found = sorted(glob.glob(os.path.join(p["binaryDir"], pat)))
            if found:
                return found[0]
        return None

    def model(self):
        e = self.environment()
        px = proxy.detect()
        out = []
        for p in self.presets:
            d = dict(p)
            d["state"] = self.gen_state(p)
            out.append(d)
        return {
            "repo": REPO, "host": env.HOST, "environment": e, "presets": out, "options": self.options,
            "samples": self.samples, "configs": self.configs, "fingerprint": self.fingerprint,
            "schema": self.schema(), "proxy": proxy.masked(px), "setupResume": setup.pending_resume(OUT),
        }

    # ------------------------------------------------------------ task helpers
    def _task_env(self, mode):
        px = proxy.detect()
        warning = ""
        if mode == "wsl":
            px, warning = proxy.wsl_rewrite(px)
        return proxy.task_env(px), warning

    def _cmd_quote(self, s, mode):
        if mode == "wsl" or not env.IS_WINDOWS:
            return "'" + s.replace("'", "'\\''") + "'"
        return '"' + s.replace('"', '\\"') + '"'

    def _defines(self, overrides, extra, mode):
        args = []
        for name, val in (overrides or {}).items():
            if val in ("ON", "OFF"):
                args.append(f"-D{name}={val}")
        for item in extra or []:
            k, v = (item.get("name") or "").strip(), (item.get("value") or "").strip()
            if k:
                args.append(self._cmd_quote(f"-D{k}={v}", mode))
        return " ".join(args)

    def _log_dir(self, p):
        return os.path.join(p["binaryDir"], "devtool-logs")

    def _cwd(self, mode="native"):
        return None if mode == "wsl" else REPO

    def _wsl_sync(self):
        """Commands mirroring the repo into the WSL filesystem (like VS ~/.vs/<proj>) and entering it.
        Builds then run on ext4 instead of the slow /mnt/c 9P mount."""
        src = env.to_wsl_path(REPO).rstrip("/") + "/"
        return [
            f"mkdir -p {WSL_ROOT}",
            f"rsync -a --delete --exclude=/out/ --exclude=/.vs/ --exclude=/.git/ {self._cmd_quote(src, 'wsl')} {WSL_ROOT}/",
            f"cd {WSL_ROOT}",
        ]

    def _stale_cache(self, p):
        """Cache from another source dir or pointing to missing vcpkg/toolchain files needs --fresh."""
        stale = False
        if p["mode"] == "native":
            vals = {}
            try:
                with open(os.path.join(p["binaryDir"], "CMakeCache.txt"), "r", encoding="utf-8", errors="replace") as f:
                    for line in f:
                        key, sep, val = line.partition("=")
                        if sep and ":" in key:
                            vals[key.split(":", 1)[0]] = val.strip()
            except OSError:
                vals = {}
            home = vals.get("CMAKE_HOME_DIRECTORY")
            if home and os.path.normcase(os.path.abspath(home)) != os.path.normcase(REPO):
                stale = True
            for key in ("VCPKG_EXECUTABLE", "CMAKE_TOOLCHAIN_FILE"):
                if vals.get(key) and not os.path.isfile(vals[key]):
                    stale = True
            inst = vals.get("VCPKG_INSTALLED_DIR")
            if inst and not os.path.normcase(os.path.abspath(inst)).startswith(os.path.normcase(REPO)):
                stale = True
        return stale

    # ------------------------------------------------------------ actions
    def generate(self, names, overrides, extra, rhi=None):
        created = []
        for name in names:
            p = self.preset(name)
            if not p or not p["available"]:
                continue
            mode = p["mode"]
            tenv, warn = self._task_env(mode)
            defines = self._defines(overrides, extra, mode)
            chosen = (rhi or {}).get(p["group"], "")
            if chosen in p["rhis"]:
                defines = f"-DVKE_RENDER_SYSTEM={chosen} {defines}".strip()
            cmds = self._wsl_sync() if mode == "wsl" else []
            if warn:
                cmds.append(("echo " + self._cmd_quote("[devtool] " + warn, mode)))
            fresh = " --fresh" if self._stale_cache(p) else ""
            if fresh:
                cmds.append("echo " + self._cmd_quote("[devtool] Stale CMake cache detected, reconfiguring with --fresh", mode))
            cmds.append(f"cmake --preset {name}{fresh} {defines}".strip())
            vs = self.environment().get("visualStudio") or {}
            fp = self.fingerprint
            ov = {"overrides": overrides, "extra": extra, "rhi": chosen}

            def done(t, p=p, fp=fp, ov=ov):
                os.makedirs(p["binaryDir"], exist_ok=True)
                with open(self.gen_state_path(p), "w", encoding="utf-8") as f:
                    json.dump({"ok": t.state == tasks.SUCCEEDED, "fingerprint": fp, "time": time.time(),
                               "log": t.logPath, **ov}, f, indent=2)
            os.makedirs(p["binaryDir"], exist_ok=True)
            t = tasks.Task("generate", f"Generate {name}", cmds, self._log_dir(p), mode=mode, preset=name,
                           cwd=self._cwd(mode),
                           task_env=tenv, on_done=done)
            created.append(self.scheduler.submit(t).id)
        return created

    def build(self, names, configs, sample_targets):
        created, skipped = [], []
        for name in names:
            p = self.preset(name)
            if not p or not p["available"] or not self.gen_state(p).get("generated"):
                skipped.append(name)
                continue
            mode = p["mode"]
            tenv, _ = self._task_env(mode)
            vs = self.environment().get("visualStudio") or {}
            targets = " ".join(f"--target {s}" for s in sample_targets) if sample_targets else ""
            for cfg in configs:
                if cfg not in p["configs"]:
                    continue
                bp = p["buildPresets"].get(cfg)
                if bp:
                    cmd = f"cmake --build --preset {bp} {targets}"
                else:
                    bdir = f"out/build/{name}" if mode == "wsl" else self._cmd_quote(p["binaryDir"], mode)
                    cmd = f"cmake --build {bdir} --config {cfg} {targets}"
                cmds = (self._wsl_sync() if mode == "wsl" else []) + [cmd.strip()]
                t = tasks.Task("build", f"Build {name} [{cfg}]", cmds, self._log_dir(p), mode=mode,
                               preset=name, config=cfg, cwd=self._cwd(mode),
                               vcvars=vs.get("vcvars64") if p["needsVcvars"] and mode == "native" else None,
                               task_env=tenv)
                created.append(self.scheduler.submit(t).id)
        return created, skipped

    def _wsl_home(self):
        if not hasattr(self, "_wsl_home_cache"):
            distro = env.wsl_status()["distroName"]
            code, out = env.run(["wsl.exe", "-d", distro, "--exec", "sh", "-c", "echo $HOME"], timeout=20)
            home = out.strip().splitlines()[-1] if code == 0 and out.strip() else ""
            self._wsl_home_cache = (distro, home)
        return self._wsl_home_cache

    def task_bin_dir(self, t):
        """Windows-accessible bin directory of the task's preset build (bin/<Config> when it exists)."""
        result = None
        p = self.preset(t.preset) if t else None
        if p:
            base = os.path.join(p["binaryDir"], "bin")
            if p["mode"] == "wsl":
                distro, home = self._wsl_home()
                if home:
                    rel = f"{home}/devtool/{os.path.basename(REPO)}/out/build/{p['name']}/bin"
                    base = f"\\\\wsl.localhost\\{distro}" + rel.replace("/", "\\")
            cfg = os.path.join(base, t.config) if t.config else base
            result = cfg if os.path.isdir(cfg) else base
        return result

    def open_path(self, path):
        ok, msg = False, ""
        if not path or not os.path.exists(path):
            msg = f"Path does not exist (build first): {path}"
        else:
            try:
                if env.IS_WINDOWS:
                    os.startfile(path)  # noqa: S606
                else:
                    subprocess.Popen(["xdg-open", path], start_new_session=True)
                ok = True
            except OSError as e:
                msg = str(e)
        return ok, msg

    def open_log(self, path):
        ok, msg = False, ""
        if not path or not os.path.isfile(path):
            msg = f"Log not found: {path}"
        elif env.IS_WINDOWS:
            subprocess.Popen(["notepad.exe", path])
            ok = True
        else:
            ok, msg = self.open_path(path)
        return ok, msg

    def open_solution(self, names):
        if len(names) != 1:
            return False, "Select exactly one Visual Studio preset to open its solution."
        p = self.preset(names[0])
        if not p or not p["isVS"]:
            return False, "The selected preset does not use the Visual Studio generator."
        sln = self.solution(p)
        if not sln or not self.gen_state(p).get("generated"):
            return False, "Solution not found. Run Generate for this preset first."
        vs = self.environment().get("visualStudio") or {}
        exe = vs.get("devenv")
        log_dir = self._log_dir(p)
        os.makedirs(log_dir, exist_ok=True)
        with open(os.path.join(log_dir, f"open-{time.strftime('%Y%m%d-%H%M%S')}.log"), "w", encoding="utf-8") as f:
            f.write(f"Opening {sln} with {exe or 'shell association'}\n")
        if exe:
            subprocess.Popen([exe, sln], creationflags=0x00000008)  # DETACHED_PROCESS
        else:
            os.startfile(sln)  # noqa: S606 - Windows-only path; VS presets require Windows
        return True, sln


APP = None
TOKEN = secrets.token_urlsafe(24)


class Handler(http.server.BaseHTTPRequestHandler):
    server_version = "VKEDevTool/1.0"

    def log_message(self, fmt, *args):  # keep console quiet
        pass

    # ------------------------------------------------------------ helpers
    def _authorized(self):
        q = urllib.parse.parse_qs(urllib.parse.urlparse(self.path).query)
        return self.headers.get("X-DevTool-Token") == TOKEN or q.get("token", [""])[0] == TOKEN

    def _json(self, obj, code=200):
        data = json.dumps(obj).encode("utf-8")
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _body(self):
        n = int(self.headers.get("Content-Length") or 0)
        if not n:
            return {}
        try:
            return json.loads(self.rfile.read(n).decode("utf-8"))
        except ValueError:
            return {}

    def _static(self, path):
        rel = "index.html" if path in ("/", "") else path.lstrip("/")
        full = os.path.abspath(os.path.join(WEB, rel))
        if not full.startswith(WEB) or not os.path.isfile(full):
            self.send_error(404)
            return
        ctype = {".html": "text/html", ".js": "application/javascript", ".css": "text/css",
                 ".svg": "image/svg+xml"}.get(os.path.splitext(full)[1], "application/octet-stream")
        with open(full, "rb") as f:
            data = f.read()
        self.send_response(200)
        self.send_header("Content-Type", ctype + "; charset=utf-8")
        self.send_header("Cache-Control", "no-store")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    # ------------------------------------------------------------ routing
    def do_GET(self):  # noqa: N802
        u = urllib.parse.urlparse(self.path)
        if not u.path.startswith("/api/"):
            return self._static(u.path)
        if u.path == "/api/ping":
            return self._json({"ok": self._authorized(), "pid": os.getpid()})
        if not self._authorized():
            return self._json({"error": "unauthorized"}, 401)
        q = urllib.parse.parse_qs(u.query)
        if u.path == "/api/model":
            return self._json(APP.model())
        if u.path == "/api/state":
            return self._json(cache.load(OUT, APP.fingerprint, APP.schema()))
        if u.path == "/api/tasks":
            return self._json({"tasks": APP.scheduler.list(), "setup": setup.progress(OUT)})
        if u.path == "/api/log":
            t = APP.scheduler.get(int(q.get("id", ["0"])[0]))
            path = t.logPath if t else None
            if not path:
                return self._json({"text": ""})
            tail = int(q.get("tail", ["200000"])[0])
            try:
                with open(path, "rb") as f:
                    f.seek(0, 2)
                    size = f.tell()
                    f.seek(max(0, size - tail))
                    text = f.read().decode("utf-8", errors="replace")
            except OSError:
                text = ""
            return self._json({"text": text, "path": path})
        if u.path == "/api/setup/check":
            groups = q.get("groups", [""])[0].split(",") if q.get("groups") else None
            if q.get("quick", [""])[0] == "1":
                return self._json(setup.check(REPO, OUT, None, groups, quick=True))
            return self._json(setup.check(REPO, OUT, APP.environment(refresh=True), groups))
        if u.path == "/api/proxy/test":
            px = proxy.detect()
            groups = ["common", "windows" if env.IS_WINDOWS else (env.linux_distro().get("pm") or "apt")]
            if env.IS_WINDOWS:
                groups.append("android")
            return self._json({"proxy": proxy.masked(px), "results": proxy.preflight(px, groups)})
        return self._json({"error": "not found"}, 404)

    def do_POST(self):  # noqa: N802
        u = urllib.parse.urlparse(self.path)
        if not self._authorized():
            return self._json({"error": "unauthorized"}, 401)
        b = self._body()
        if u.path == "/api/state":
            cache.save(OUT, APP.fingerprint, APP.schema(), b)
            return self._json({"ok": True})
        if u.path == "/api/reload":
            APP.reload()
            return self._json(APP.model())
        if u.path == "/api/generate":
            ids = APP.generate(b.get("presets", []), b.get("overrides", {}), b.get("extra", []), b.get("rhi", {}))
            return self._json({"tasks": ids})
        if u.path == "/api/build":
            ids, skipped = APP.build(b.get("presets", []), b.get("configs", []), b.get("samples", []))
            return self._json({"tasks": ids, "skipped": skipped})
        if u.path == "/api/open":
            ok, msg = APP.open_solution(b.get("presets", []))
            return self._json({"ok": ok, "message": msg}, 200 if ok else 400)
        if u.path == "/api/tasks/cancel":
            return self._json({"ok": APP.scheduler.cancel(int(b.get("id", 0)))})
        if u.path in ("/api/tasks/openlog", "/api/tasks/openbin", "/api/tasks/remove"):
            t = APP.scheduler.get(int(b.get("id", 0)))
            if u.path == "/api/tasks/remove":
                ok, msg = APP.scheduler.remove(int(b.get("id", 0))), "Cancel the task before removing it."
            elif u.path == "/api/tasks/openlog":
                ok, msg = APP.open_log(t.logPath if t else None)
            else:
                ok, msg = APP.open_path(APP.task_bin_dir(t))
            return self._json({"ok": ok, "message": msg}, 200 if ok else 400)
        if u.path == "/api/tasks/clear":
            APP.scheduler.clear_finished()
            return self._json({"ok": True})
        if u.path == "/api/proxy":
            proxy.set_manual(b.get("http", ""), b.get("https", ""), b.get("noProxy", ""))
            return self._json({"proxy": proxy.masked(proxy.detect())})
        if u.path == "/api/setup/run":
            if APP.scheduler.busy({"setup"}):
                return self._json({"error": "Setup is already running."}, 409)
            res = setup.run(REPO, OUT, APP.scheduler, APP.environment(refresh=True), b.get("ids", []),
                            proxy.detect())
            return self._json(res)
        if u.path == "/api/setup/gate":
            setup.confirm_gate(OUT, b.get("id", ""))
            return self._json({"ok": True})
        return self._json({"error": "not found"}, 404)


class Server(socketserver.ThreadingMixIn, http.server.HTTPServer):
    daemon_threads = True
    allow_reuse_address = False  # never steal a port that is in use


def _pid_alive(pid):
    if env.IS_WINDOWS:
        code, out = env.run(["tasklist", "/FI", f"PID eq {pid}", "/NH"])
        return code == 0 and str(pid) in out
    try:
        os.kill(pid, 0)
        return True
    except OSError:
        return False


def existing_instance():
    try:
        with open(SERVER_JSON, "r", encoding="utf-8") as f:
            info = json.load(f)
    except (OSError, ValueError):
        return None
    if not _pid_alive(int(info.get("pid", 0))):
        return None
    try:
        url = f"http://127.0.0.1:{info['port']}/api/ping?token={info['token']}"
        opener = urllib.request.build_opener(urllib.request.ProxyHandler({}))  # never proxy localhost
        with opener.open(url, timeout=3) as r:
            if json.loads(r.read().decode("utf-8")).get("ok"):
                return info
    except (OSError, ValueError, KeyError):
        pass
    return None


def stop_instance(info):
    """Terminates a running DevTool server and waits until it is gone. Returns True on success."""
    pid = int(info.get("pid", 0))
    if env.IS_WINDOWS:
        env.run(["taskkill", "/PID", str(pid), "/T", "/F"])
    else:
        try:
            os.kill(pid, signal.SIGTERM)
        except OSError:
            pass
    deadline = time.time() + 10
    while _pid_alive(pid) and time.time() < deadline:
        time.sleep(0.2)
    return not _pid_alive(pid)


def main():
    global APP, TOKEN
    ap = argparse.ArgumentParser(description="VKE DevTool")
    ap.add_argument("--no-browser", action="store_true")
    ap.add_argument("--port", type=int, default=0, help="Preferred port; 0 = OS assigned. Fails if occupied.")
    ap.add_argument("--restart", action="store_true", help="Stop a running instance and start a fresh one.")
    args = ap.parse_args()

    os.makedirs(OUT, exist_ok=True)
    inst = existing_instance()
    if inst and args.restart:
        print(f"[devtool] Stopping running instance (PID {inst['pid']})...")
        if not stop_instance(inst):
            print(f"[devtool] Could not stop PID {inst['pid']}. Close it manually and relaunch.")
            return 1
        inst = None
    if inst:
        url = f"http://127.0.0.1:{inst['port']}/?token={inst['token']}"
        print(f"[devtool] Already running (PID {inst['pid']}): {url}")
        if not args.no_browser:
            webbrowser.open(url)
        return 0
    try:
        os.remove(SERVER_JSON)
    except OSError:
        pass

    APP = App()
    # Reuse the previous port and token so browser tabs left open reconnect after a restart.
    prev = {}
    try:
        with open(SESSION_JSON, "r", encoding="utf-8") as f:
            prev = json.load(f)
    except (OSError, ValueError):
        prev = {}
    if prev.get("token"):
        TOKEN = prev["token"]
    candidates = [args.port] if args.port else [int(prev.get("port") or 0), 0]
    srv, err = None, None
    for cand in dict.fromkeys(candidates):
        try:
            srv = Server(("127.0.0.1", cand), Handler)
            break
        except OSError as e:
            err = e
    if srv is None:
        print(f"[devtool] Cannot bind 127.0.0.1:{args.port}: {err}. Use --port 0 for an automatic free port.")
        return 1
    port = srv.server_address[1]
    with open(SESSION_JSON, "w", encoding="utf-8") as f:
        json.dump({"port": port, "token": TOKEN}, f)
    with open(SERVER_JSON, "w", encoding="utf-8") as f:
        json.dump({"pid": os.getpid(), "port": port, "token": TOKEN, "started": time.time()}, f)
    url = f"http://127.0.0.1:{port}/?token={TOKEN}"
    print(f"[devtool] Repository: {REPO}")
    print(f"[devtool] Serving on {url}")
    print("[devtool] Press Ctrl+C to stop.")
    if not args.no_browser:
        threading.Timer(0.5, lambda: webbrowser.open(url)).start()
    try:
        srv.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        srv.server_close()
        try:
            os.remove(SERVER_JSON)
        except OSError:
            pass
    return 0


if __name__ == "__main__":
    sys.exit(main())

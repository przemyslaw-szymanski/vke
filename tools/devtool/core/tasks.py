"""Task model and FIFO scheduler. Each task runs in its own visible terminal and is logged to a file.

Task kinds: setup, generate, build, open (future: test). A task executes a list of shell command strings in
one of the modes: 'native' (cmd on Windows / bash on Linux), 'wsl' (bash inside WSL), 'elevated' (Windows,
single UAC prompt for the whole script).
"""
import itertools
import os
import subprocess
import threading
import time

from . import env

WAITING, RUNNING, SUCCEEDED, FAILED, CANCELLED = "waiting", "running", "succeeded", "failed", "cancelled"
_ids = itertools.count(1)
_NEW_CONSOLE = 0x00000010 if env.IS_WINDOWS else 0
_NO_WINDOW = 0x08000000 if env.IS_WINDOWS else 0


class Task:
    def __init__(self, kind, title, commands, log_dir, mode="native", preset="", config="", cwd=None,
                 vcvars=None, task_env=None, on_done=None, meta=None):
        self.id = next(_ids)
        self.kind = kind
        self.title = title
        self.commands = commands
        self.mode = mode
        self.preset = preset
        self.config = config
        self.cwd = cwd
        self.vcvars = vcvars
        self.env = task_env
        self.on_done = on_done
        self.meta = meta or {}
        self.state = WAITING
        self.exitCode = None
        self.createdAt = time.time()
        self.startedAt = None
        self.endedAt = None
        self.error = ""
        stamp = time.strftime("%Y%m%d-%H%M%S")
        suffix = f"-{config}" if config else ""
        os.makedirs(log_dir, exist_ok=True)
        base = os.path.join(log_dir, f"{kind}{suffix}-{stamp}-{self.id}")
        self.logPath = base + ".log"
        self._exit_path = base + ".exit"
        self._script_base = base
        self._proc = None
        self._cancel = False

    def to_json(self):
        now = time.time()
        return {
            "id": self.id, "kind": self.kind, "title": self.title, "preset": self.preset, "config": self.config,
            "mode": self.mode, "state": self.state, "exitCode": self.exitCode, "error": self.error,
            "createdAt": self.createdAt, "startedAt": self.startedAt, "endedAt": self.endedAt,
            "elapsed": ((self.endedAt or now) - self.startedAt) if self.startedAt else 0,
            "logPath": self.logPath,
        }

    # ------------------------------------------------------------ script generation
    def _write_windows(self):
        """PowerShell wrapper: runs commands via cmd (optionally after vcvars), tees to log, writes exit code."""
        lines = []
        if self.vcvars:
            lines.append(f'call "{self.vcvars}" >nul')
            lines.append("if errorlevel 1 exit /b 1")
        for c in self.commands:
            lines.append(c)
            lines.append("if errorlevel 1 exit /b %errorlevel%")
        cmd_path = self._script_base + ".cmd"
        with open(cmd_path, "w", encoding="utf-8", newline="\r\n") as f:
            f.write("@echo off\nchcp 65001 >nul\n")
            if self.cwd:
                f.write(f'cd /d "{self.cwd}"\n')
            f.write("\n".join(lines) + "\nexit /b 0\n")
        ps_path = self._script_base + ".ps1"
        title = self.title.replace("'", "''")
        log = self.logPath.replace("'", "''")
        ex = self._exit_path.replace("'", "''")
        cmdp = cmd_path.replace("'", "''")
        keep_open = "elevated" == self.mode
        with open(ps_path, "w", encoding="utf-8-sig", newline="\r\n") as f:
            f.write(f"$Host.UI.RawUI.WindowTitle = 'VKE DevTool - {title}'\n")
            f.write("[Console]::OutputEncoding = [System.Text.Encoding]::UTF8\n")
            f.write(f"& cmd.exe /d /c \"`\"{cmdp}`\" 2>&1\" | Tee-Object -FilePath '{log}'\n")
            f.write("$code = $LASTEXITCODE\n")
            f.write(f"Set-Content -Path '{ex}' -Value $code -Encoding ascii\n")
            f.write("if ($code -ne 0) { Write-Host \"`nFAILED with exit code $code\" -ForegroundColor Red; "
                    "Read-Host 'Press Enter to close' | Out-Null }\n")
            if keep_open:
                f.write("else { Write-Host \"`nDone.\" -ForegroundColor Green; Start-Sleep -Seconds 3 }\n")
        return ps_path

    def _write_bash(self, to_path):
        """Bash wrapper used for WSL and native Linux. Paths inside script are translated by to_path."""
        sh_path = self._script_base + ".sh"
        log, ex = to_path(self.logPath), to_path(self._exit_path)
        body = " && \\\n  ".join(f"{{ {c}; }}" for c in self.commands) or "true"
        cwd = f'cd "{to_path(self.cwd)}" || exit 1\n' if self.cwd else ""
        with open(sh_path, "w", encoding="utf-8", newline="\n") as f:
            f.write("#!/usr/bin/env bash\nset -o pipefail\n")
            f.write(f"printf '\\033]0;VKE DevTool - {self.title}\\007'\n")
            f.write(cwd)
            f.write(f"( {body} ) 2>&1 | tee \"{log}\"\n")
            f.write("code=${PIPESTATUS[0]}\n")
            f.write(f"echo $code > \"{ex}\"\n")
            f.write("if [ $code -ne 0 ]; then echo; echo \"FAILED with exit code $code\"; "
                    "read -r -p 'Press Enter to close' _; fi\n")
        return sh_path

    # ------------------------------------------------------------ execution
    def start(self):
        if self.mode == "wsl":
            sh = self._write_bash(env.to_wsl_path)
            distro = env.wsl_status()["distroName"]
            argv = ["wsl.exe", "-d", distro, "--", "bash", env.to_wsl_path(sh)]
            self._proc = subprocess.Popen(argv, creationflags=_NEW_CONSOLE, env=self.env)
        elif env.IS_WINDOWS:
            ps = self._write_windows()
            if self.mode == "elevated":
                inner = f"-NoProfile -ExecutionPolicy Bypass -File \"{ps}\""
                argv = ["powershell", "-NoProfile", "-Command",
                        f"Start-Process powershell -Verb RunAs -Wait -ArgumentList '{inner}'"]
                self._proc = subprocess.Popen(argv, creationflags=_NO_WINDOW, env=self.env)
            else:
                argv = ["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", ps]
                self._proc = subprocess.Popen(argv, creationflags=_NEW_CONSOLE, env=self.env)
        else:
            sh = self._write_bash(lambda p: p)
            term = env.linux_terminal()
            if term:
                self._proc = subprocess.Popen(term + ["bash", sh], env=self.env, start_new_session=True)
            else:
                self._proc = subprocess.Popen(["bash", sh], env=self.env, stdin=subprocess.DEVNULL,
                                              stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                              start_new_session=True)

    def wait(self):
        """Polls exit file; terminal emulators may return before the script ends."""
        grace = None
        while True:
            if self._cancel:
                return -2
            if os.path.isfile(self._exit_path):
                time.sleep(0.2)
                try:
                    with open(self._exit_path, "r", encoding="ascii", errors="ignore") as f:
                        return int(f.read().strip() or "1")
                except (OSError, ValueError):
                    return 1
            if self._proc and self._proc.poll() is not None:
                grace = grace or time.time()
                if time.time() - grace > 5:
                    self.error = "Terminal closed before the task finished (or UAC was declined)."
                    return 1
            time.sleep(0.5)

    def kill(self):
        self._cancel = True
        p = self._proc
        if not p:
            return
        if env.IS_WINDOWS:
            env.run(["taskkill", "/T", "/F", "/PID", str(p.pid)])
        else:
            try:
                os.killpg(os.getpgid(p.pid), 15)
            except OSError:
                pass


class Scheduler:
    def __init__(self):
        self._tasks = []
        self._lock = threading.Lock()
        self._cv = threading.Condition(self._lock)
        threading.Thread(target=self._worker, daemon=True, name="devtool-scheduler").start()

    def submit(self, task):
        with self._cv:
            self._tasks.append(task)
            self._cv.notify()
        return task

    def list(self):
        with self._lock:
            return [t.to_json() for t in self._tasks]

    def get(self, tid):
        with self._lock:
            return next((t for t in self._tasks if t.id == tid), None)

    def cancel(self, tid):
        t = self.get(tid)
        if not t:
            return False
        with self._lock:
            if t.state == WAITING:
                t.state = CANCELLED
                t.endedAt = time.time()
                return True
        if t.state == RUNNING:
            t.kill()
            return True
        return False

    def clear_finished(self):
        with self._lock:
            self._tasks = [t for t in self._tasks if t.state in (WAITING, RUNNING)]

    def remove(self, tid):
        """Removes a finished task row; active tasks must be cancelled first."""
        with self._lock:
            before = len(self._tasks)
            self._tasks = [t for t in self._tasks if t.id != tid or t.state in (WAITING, RUNNING)]
            return len(self._tasks) != before

    def busy(self, kinds=None):
        with self._lock:
            return any(t.state in (WAITING, RUNNING) and (kinds is None or t.kind in kinds) for t in self._tasks)

    def _next(self):
        with self._cv:
            while True:
                t = next((t for t in self._tasks if t.state == WAITING), None)
                if t:
                    t.state = RUNNING
                    t.startedAt = time.time()
                    return t
                self._cv.wait()

    def _worker(self):
        while True:
            t = self._next()
            try:
                t.start()
                code = t.wait()
            except Exception as e:  # noqa: BLE001 - report any launch failure on the task
                t.error = str(e)
                code = 1
            t.exitCode = code
            t.endedAt = time.time()
            t.state = CANCELLED if code == -2 else (SUCCEEDED if code == 0 else FAILED)
            if t.on_done:
                try:
                    t.on_done(t)
                except Exception as e:  # noqa: BLE001
                    t.error = (t.error + " " + str(e)).strip()

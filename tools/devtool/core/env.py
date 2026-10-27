"""Host environment detection: OS, Visual Studio, WSL, Linux distro, terminal emulators."""
import os
import platform
import re
import shutil
import subprocess

IS_WINDOWS = os.name == "nt"
HOST = "Windows" if IS_WINDOWS else "Linux"
WSL_DISTRO = os.environ.get("VKE_DEVTOOL_WSL_DISTRO", "Ubuntu")
_CREATE_NO_WINDOW = 0x08000000 if IS_WINDOWS else 0


def run(cmd, timeout=30, env=None, shell=False):
    """Run a command silently, return (exit_code, stdout+stderr text)."""
    try:
        p = subprocess.run(cmd, capture_output=True, timeout=timeout, env=env, shell=shell,
                           creationflags=_CREATE_NO_WINDOW)
        out = p.stdout + p.stderr
        # wsl.exe emits UTF-16LE
        if b"\x00" in out:
            text = out.decode("utf-16-le", errors="replace")
        else:
            text = out.decode("utf-8", errors="replace")
        return p.returncode, text.replace("\x00", "")
    except (OSError, subprocess.TimeoutExpired) as e:
        return -1, str(e)


# ---------------------------------------------------------------- MSYS2
MSYS2_DEFAULT = r"C:\msys64"
_msys2_cache = []


def _is_msys2(root):
    return bool(root) and os.path.isfile(os.path.join(root, "usr", "bin", "bash.exe"))


def _msys2_candidates():
    for var in ("MSYS2_ROOT", "MSYS2_PATH", "MSYS64_ROOT"):
        yield os.environ.get(var, "")
    # PATH entries such as <root>\ucrt64\bin, <root>\mingw64\bin, <root>\usr\bin
    for entry in os.environ.get("PATH", "").split(os.pathsep):
        entry = entry.strip().rstrip("\\/")
        if os.path.basename(entry).lower() == "bin":
            yield os.path.dirname(os.path.dirname(entry))
    for exe in ("g++", "gcc", "bash", "pacman"):
        p = shutil.which(exe)
        if p:
            yield os.path.dirname(os.path.dirname(os.path.dirname(p)))
            yield os.path.dirname(os.path.dirname(p))
    # Installer registers itself in the Uninstall keys
    try:
        import winreg
        for root in (winreg.HKEY_CURRENT_USER, winreg.HKEY_LOCAL_MACHINE):
            try:
                with winreg.OpenKey(root, r"Software\Microsoft\Windows\CurrentVersion\Uninstall") as k:
                    for i in range(winreg.QueryInfoKey(k)[0]):
                        name = winreg.EnumKey(k, i)
                        try:
                            with winreg.OpenKey(k, name) as sk:
                                if "msys2" in str(winreg.QueryValueEx(sk, "DisplayName")[0]).lower():
                                    yield winreg.QueryValueEx(sk, "InstallLocation")[0]
                        except OSError:
                            pass
            except OSError:
                pass
    except ImportError:
        pass
    for drive in "CDEFGH":
        for d in ("msys64", "msys2", "tools\\msys64", "tools\\msys2"):
            yield f"{drive}:\\{d}"


def msys2_root():
    """Detected MSYS2 installation root or None."""
    if not IS_WINDOWS:
        return None
    if _msys2_cache and _is_msys2(_msys2_cache[0]):
        return _msys2_cache[0]
    _msys2_cache.clear()
    for c in _msys2_candidates():
        if _is_msys2(c):
            _msys2_cache.append(os.path.normpath(c))
            return _msys2_cache[0]
    return None


# ---------------------------------------------------------------- vcpkg
_REPO = os.path.abspath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "..", ".."))
VCPKG_SEARCH_FILE = os.path.join(_REPO, "tools", "devtool", "vcpkg_search_paths.txt")
VCPKG_DEFAULT_WINDOWS = r"C:\vcpkg"
VCPKG_DEFAULT_LINUX = "$HOME/vcpkg"


def vcpkg_search_paths():
    """Raw lines of cmake/VcpkgSearchPaths.txt (same list CMakeLists.txt uses)."""
    try:
        with open(VCPKG_SEARCH_FILE, "r", encoding="utf-8") as f:
            return [l.strip() for l in f if l.strip() and not l.lstrip().startswith("#")]
    except OSError:
        return []


def vcpkg_root():
    """Host vcpkg root using the CMake search order: VCPKG_ROOT, PATH, VcpkgSearchPaths.txt."""
    exe = "vcpkg.exe" if IS_WINDOWS else "vcpkg"
    cands = [os.environ.get("VCPKG_ROOT", "")]
    p = shutil.which("vcpkg")
    if p:
        cands.append(os.path.dirname(os.path.realpath(p)))
    for line in vcpkg_search_paths():
        if re.search(r"\$ENV\{(\w+)\}", line) and not all(os.environ.get(n) for n in re.findall(r"\$ENV\{(\w+)\}", line)):
            continue
        cands.append(re.sub(r"\$ENV\{(\w+)\}", lambda m: os.environ.get(m.group(1), ""), line))
    for c in cands:
        if c and os.path.isfile(os.path.join(c, exe)):
            return os.path.normpath(c)
    return None


def vcpkg_shell_find():
    """Bash snippet printing the vcpkg root found with the same search order (for Linux / WSL checks)."""
    dirs = []
    for line in vcpkg_search_paths():
        if line[:1] == "/" or line.startswith("$ENV{HOME}"):
            dirs.append(re.sub(r"\$ENV\{(\w+)\}", r"$\1", line))
    lst = " ".join(dirs)
    # No double quotes: wsl.exe re-joins and re-quotes arguments and breaks nested quoting.
    return ("for d in $VCPKG_ROOT $(command -v vcpkg | xargs -r readlink -f | xargs -r dirname) " +
            lst + "; do test -x $d/vcpkg && echo VCPKG=$d && exit 0; done; exit 1")


# ---------------------------------------------------------------- Android SDK
def android_sdk_root():
    """Existing SDK from ANDROID_HOME / ANDROID_SDK_ROOT (process or machine/user registry), else the
    system-wide default <ProgramFiles>\\Android\\Sdk."""
    names = ("ANDROID_HOME", "ANDROID_SDK_ROOT")
    cands = [os.environ.get(n, "") for n in names]
    if IS_WINDOWS:
        import winreg
        for root, key in ((winreg.HKEY_LOCAL_MACHINE, r"SYSTEM\CurrentControlSet\Control\Session Manager\Environment"),
                          (winreg.HKEY_CURRENT_USER, r"Environment")):
            for n in names:
                try:
                    with winreg.OpenKey(root, key) as k:
                        cands.append(os.path.expandvars(winreg.QueryValueEx(k, n)[0]))
                except OSError:
                    pass
    result = ""
    for c in cands:
        if not result and c and os.path.isdir(c):
            result = os.path.normpath(c)
    if not result:
        if IS_WINDOWS:
            result = os.path.join(os.environ.get("ProgramFiles", r"C:\Program Files"), "Android", "Sdk")
        else:
            result = "/opt/android-sdk"
    return result


# ---------------------------------------------------------------- Visual Studio
def vswhere_path():
    if not IS_WINDOWS:
        return None
    base = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    p = os.path.join(base, "Microsoft Visual Studio", "Installer", "vswhere.exe")
    return p if os.path.isfile(p) else None


def visual_studio():
    """Returns dict with installationPath, version, devenv, vcvars64 or None."""
    vw = vswhere_path()
    if not vw:
        return None
    code, out = run([vw, "-latest", "-prerelease", "-products", "*", "-format", "value",
                     "-property", "installationPath"])
    path = out.strip().splitlines()[0] if code == 0 and out.strip() else ""
    if not path:
        return None
    _, ver = run([vw, "-latest", "-prerelease", "-products", "*", "-format", "value",
                  "-property", "installationVersion"])
    devenv = os.path.join(path, "Common7", "IDE", "devenv.exe")
    vcvars = os.path.join(path, "VC", "Auxiliary", "Build", "vcvars64.bat")
    return {
        "installationPath": path,
        "version": ver.strip().splitlines()[0] if ver.strip() else "",
        "devenv": devenv if os.path.isfile(devenv) else None,
        "vcvars64": vcvars if os.path.isfile(vcvars) else None,
        "installer": os.path.join(os.path.dirname(vswhere_path()), "vs_installer.exe"),
    }


# ---------------------------------------------------------------- WSL
def wsl_status():
    """Returns {'installed': bool, 'distro': bool, 'userReady': bool, 'distroName': str}."""
    st = {"installed": False, "distro": False, "userReady": False, "distroName": WSL_DISTRO}
    if not IS_WINDOWS or not shutil.which("wsl.exe"):
        return st
    code, out = run(["wsl.exe", "-l", "-q"], timeout=20)
    if code != 0:
        return st
    st["installed"] = True
    names = [l.strip() for l in out.splitlines() if l.strip()]
    match = next((n for n in names if n.lower() == WSL_DISTRO.lower()), None)
    if not match:
        match = next((n for n in names if n.lower().startswith(WSL_DISTRO.lower())), None)
    if not match:
        return st
    st["distro"] = True
    st["distroName"] = match
    # A distro whose default user is root (no user created yet) is not ready.
    code, out = run(["wsl.exe", "-d", match, "--", "id", "-u"], timeout=30)
    st["userReady"] = code == 0 and out.strip().isdigit() and out.strip() != "0"
    return st


def to_wsl_path(win_path):
    p = os.path.abspath(win_path).replace("\\", "/")
    if len(p) > 1 and p[1] == ":":
        return f"/mnt/{p[0].lower()}{p[2:]}"
    return p


# ---------------------------------------------------------------- Linux
def linux_distro():
    """Returns {'id', 'like', 'pm': 'apt'|'pacman'|None}."""
    info = {"id": "", "like": "", "pm": None}
    try:
        with open("/etc/os-release", "r", encoding="utf-8") as f:
            for line in f:
                k, _, v = line.strip().partition("=")
                v = v.strip('"')
                if k == "ID":
                    info["id"] = v
                elif k == "ID_LIKE":
                    info["like"] = v
    except OSError:
        return info
    ids = f" {info['id']} {info['like']} "
    if " arch " in ids or " manjaro " in ids or " endeavouros " in ids:
        info["pm"] = "pacman"
    elif " debian " in ids or " ubuntu " in ids:
        info["pm"] = "apt"
    return info


def linux_terminal():
    """Returns argv prefix for running a command in a new terminal window, or None."""
    if IS_WINDOWS or not os.environ.get("DISPLAY") and not os.environ.get("WAYLAND_DISPLAY"):
        return None
    candidates = [
        ("x-terminal-emulator", ["x-terminal-emulator", "-e"]),
        ("gnome-terminal", ["gnome-terminal", "--wait", "--"]),
        ("konsole", ["konsole", "-e"]),
        ("xfce4-terminal", ["xfce4-terminal", "-x"]),
        ("alacritty", ["alacritty", "-e"]),
        ("kitty", ["kitty"]),
        ("xterm", ["xterm", "-e"]),
    ]
    for exe, argv in candidates:
        if shutil.which(exe):
            return argv
    return None


def summary():
    s = {"host": HOST, "platform": platform.platform()}
    if IS_WINDOWS:
        s["visualStudio"] = visual_studio()
        s["wsl"] = wsl_status()
    else:
        s["distro"] = linux_distro()
        s["terminal"] = linux_terminal()
    return s

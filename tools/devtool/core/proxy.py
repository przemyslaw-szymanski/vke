"""Proxy detection, connectivity pre-flight, and per-tool proxy propagation."""
import os
import re
import socket
import urllib.error
import urllib.parse
import urllib.request

from . import env

PREFLIGHT_URLS = {
    "common": ["https://github.com", "https://objects.githubusercontent.com"],
    "windows": ["https://cdn.winget.microsoft.com/cache/source.msix", "https://aka.ms"],
    "android": ["https://dl.google.com"],
    "apt": ["http://archive.ubuntu.com/ubuntu/"],
    "pacman": ["https://geo.mirror.pkgbuild.com/"],
}

_manual = {"http": "", "https": "", "noProxy": ""}


def set_manual(http="", https="", no_proxy=""):
    _manual.update({"http": http.strip(), "https": (https or http).strip(), "noProxy": no_proxy.strip()})


def _normalize(url):
    if url and "://" not in url:
        url = "http://" + url
    return url


def _from_env():
    def g(*names):
        for n in names:
            v = os.environ.get(n)
            if v:
                return v
        return ""
    return {"http": g("HTTP_PROXY", "http_proxy"), "https": g("HTTPS_PROXY", "https_proxy"),
            "noProxy": g("NO_PROXY", "no_proxy")}


def _from_windows():
    res = {"http": "", "https": "", "noProxy": "", "pac": ""}
    try:
        import winreg
        k = winreg.OpenKey(winreg.HKEY_CURRENT_USER,
                           r"Software\Microsoft\Windows\CurrentVersion\Internet Settings")
        def q(name):
            try:
                return winreg.QueryValueEx(k, name)[0]
            except OSError:
                return None
        pac = q("AutoConfigURL")
        if pac:
            res["pac"] = pac
        if q("ProxyEnable"):
            server = q("ProxyServer") or ""
            if "=" in server:
                for part in server.split(";"):
                    proto, _, addr = part.partition("=")
                    if proto.lower() in ("http", "https"):
                        res[proto.lower()] = _normalize(addr)
            elif server:
                res["http"] = res["https"] = _normalize(server)
            override = q("ProxyOverride") or ""
            res["noProxy"] = ",".join(x.strip() for x in override.split(";") if x.strip() and x.strip() != "<local>")
    except OSError:
        pass
    if not res["http"]:
        code, out = env.run(["netsh", "winhttp", "show", "proxy"])
        m = re.search(r"Proxy Server\(s\)\s*:\s*(\S+)", out)
        if code == 0 and m:
            res["http"] = res["https"] = _normalize(m.group(1))
            b = re.search(r"Bypass List\s*:\s*(\S+)", out)
            if b and "(none)" not in b.group(1):
                res["noProxy"] = b.group(1).replace(";", ",")
    return res


def _from_gnome():
    res = {"http": "", "https": "", "noProxy": ""}
    if not env.shutil.which("gsettings"):
        return res
    code, mode = env.run(["gsettings", "get", "org.gnome.system.proxy", "mode"])
    if code != 0 or "manual" not in mode:
        return res
    for proto in ("http", "https"):
        _, host = env.run(["gsettings", "get", f"org.gnome.system.proxy.{proto}", "host"])
        _, port = env.run(["gsettings", "get", f"org.gnome.system.proxy.{proto}", "port"])
        host = host.strip().strip("'")
        if host:
            res[proto] = _normalize(f"{host}:{port.strip()}")
    return res


def detect():
    """Returns {'http','https','noProxy','source','pac','warnings'}."""
    warnings = []
    if _manual["http"] or _manual["https"]:
        r = dict(_manual)
        r.update(source="manual", pac="")
    else:
        r = _from_env()
        r["source"] = "environment" if (r["http"] or r["https"]) else ""
        r["pac"] = ""
        if not r["source"]:
            sys_r = _from_windows() if env.IS_WINDOWS else _from_gnome()
            if sys_r.get("http") or sys_r.get("https"):
                r.update({k: sys_r[k] for k in ("http", "https", "noProxy")})
                r["source"] = "system"
            r["pac"] = sys_r.get("pac", "")
            if r["pac"] and not r["source"]:
                warnings.append(f"A PAC script is configured ({r['pac']}). PAC cannot be resolved automatically - "
                                "set the proxy manually in Network settings if downloads fail.")
        if not r["https"]:
            r["https"] = r["http"]
    if not r["source"]:
        r["source"] = "direct"
    nop = [x for x in r.get("noProxy", "").split(",") if x]
    for local in ("localhost", "127.0.0.1"):
        if local not in nop:
            nop.append(local)
    r["noProxy"] = ",".join(nop)
    r["warnings"] = warnings
    return r


def masked(p):
    def m(u):
        return re.sub(r"//([^:@/]+):([^@/]+)@", r"//\1:***@", u or "")
    out = dict(p)
    out["http"], out["https"] = m(p.get("http")), m(p.get("https"))
    return out


def task_env(p, base=None):
    """Environment dict with proxy variables for child processes."""
    e = dict(base if base is not None else os.environ)
    if p.get("http"):
        e["HTTP_PROXY"] = e["http_proxy"] = p["http"]
    if p.get("https"):
        e["HTTPS_PROXY"] = e["https_proxy"] = p["https"]
    if p.get("noProxy"):
        e["NO_PROXY"] = e["no_proxy"] = p["noProxy"]
    if env.IS_WINDOWS:
        wslenv = [x for x in e.get("WSLENV", "").split(":") if x]
        for n in ("HTTP_PROXY/u", "HTTPS_PROXY/u", "NO_PROXY/u", "http_proxy/u", "https_proxy/u", "no_proxy/u"):
            if n.split("/")[0] in e and n not in wslenv:
                wslenv.append(n)
        e["WSLENV"] = ":".join(wslenv)
    return e


def wsl_rewrite(p):
    """Rewrite localhost proxies to the Windows host IP for WSL2 NAT mode. Returns (proxy, warning)."""
    def is_local(u):
        h = urllib.parse.urlparse(u).hostname or ""
        return h in ("localhost", "127.0.0.1", "::1")
    if not any(is_local(p.get(k, "")) for k in ("http", "https")):
        return p, ""
    cfg = os.path.join(os.path.expanduser("~"), ".wslconfig")
    try:
        with open(cfg, "r", encoding="utf-8") as f:
            if re.search(r"networkingMode\s*=\s*mirrored", f.read(), re.I):
                return p, ""
    except OSError:
        pass
    code, out = env.run(["wsl.exe", "-d", env.WSL_DISTRO, "--exec", "sh", "-c", "ip route | awk '/default/ {print $3}'"])
    gw = out.strip().splitlines()[0] if code == 0 and out.strip() else ""
    if not gw:
        return p, "Proxy points to localhost but the WSL host IP could not be determined."
    q = dict(p)
    for k in ("http", "https"):
        if is_local(q.get(k, "")):
            u = urllib.parse.urlparse(q[k])
            q[k] = u._replace(netloc=u.netloc.replace(u.hostname, gw)).geturl()
    return q, (f"Proxy uses localhost; rewritten to {gw} for WSL (NAT mode). The proxy must accept "
               "connections from WSL, or set networkingMode=mirrored in %USERPROFILE%\\.wslconfig.")


def apt_args(p):
    a = []
    if p.get("http"):
        a += ["-o", f"Acquire::http::Proxy={p['http']}"]
    if p.get("https"):
        a += ["-o", f"Acquire::https::Proxy={p['https']}"]
    return a


def sdkmanager_args(p):
    u = urllib.parse.urlparse(p.get("https") or p.get("http") or "")
    if not u.hostname:
        return []
    return ["--proxy=http", f"--proxy_host={u.hostname}", f"--proxy_port={u.port or 80}"]


def opener(p):
    handlers = {}
    if p.get("http"):
        handlers["http"] = p["http"]
    if p.get("https"):
        handlers["https"] = p["https"]
    return urllib.request.build_opener(urllib.request.ProxyHandler(handlers))


def preflight(p, groups=("common",)):
    """Checks reachability of required endpoints. Returns list of {'url','ok','error'}."""
    o = opener(p)
    results = []
    urls = []
    for g in groups:
        urls += PREFLIGHT_URLS.get(g, [])
    for url in urls:
        try:
            req = urllib.request.Request(url, method="HEAD", headers={"User-Agent": "vke-devtool"})
            o.open(req, timeout=10)
            results.append({"url": url, "ok": True, "error": ""})
        except urllib.error.HTTPError as e:
            # Any HTTP response means the network path works.
            results.append({"url": url, "ok": e.code != 407, "error": "" if e.code != 407 else
                            "Proxy authentication required (407). Use http://user:pass@host:port."})
        except (urllib.error.URLError, socket.timeout, OSError) as e:
            via = p.get("https") or p.get("http") or "direct connection"
            results.append({"url": url, "ok": False,
                            "error": f"Unreachable via {masked({'http': via})['http']}: {getattr(e, 'reason', e)}"})
    return results

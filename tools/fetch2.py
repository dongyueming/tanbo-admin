import os
import sys
import time
import urllib.request

HERE = os.path.dirname(os.path.abspath(__file__))
TARGET = os.path.join(HERE, "cloudflared.exe")
RAW = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe"

MIRRORS = [
    "https://ghfast.top/",
    "https://gh-proxy.com/",
    "https://ghproxy.net/",
    "https://github.moeyy.xyz/",
    "https://gh.llkk.cc/",
    "https://hub.gitmirror.com/",
    "",
]

probe = 1024 * 1024  # 1MB 试读

best = None
best_speed = 0.0

for m in MIRRORS:
    url = (m + RAW) if m else RAW
    label = m or "direct"
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
        t0 = time.time()
        with urllib.request.urlopen(req, timeout=15) as r:
            data = r.read(probe)
        dt = max(time.time() - t0, 0.001)
        speed = len(data) / dt
        print("%-28s %6d KB  %6d KB/s" % (label, len(data) // 1024, int(speed // 1024)), flush=True)
        if speed > best_speed:
            best_speed = speed
            best = m
    except Exception as e:
        print("%-28s fail: %s" % (label, type(e).__name__), flush=True)

if best is None:
    print("no mirror reachable", flush=True)
    sys.exit(1)

url = (best + RAW) if best else RAW
print("chosen:", best or "direct", "~", int(best_speed // 1024), "KB/s", flush=True)

req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
total = 0
with urllib.request.urlopen(req, timeout=30) as r, open(TARGET, "wb") as f:
    while True:
        chunk = r.read(1024 * 128)
        if not chunk:
            break
        f.write(chunk)
        total += len(chunk)
print("done size=", os.path.getsize(TARGET), flush=True)

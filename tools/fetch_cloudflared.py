import os
import sys
import urllib.request

TARGET = os.path.join(os.path.dirname(os.path.abspath(__file__)), "cloudflared.exe")
RAW = "https://github.com/cloudflare/cloudflared/releases/latest/download/cloudflared-windows-amd64.exe"

MIRRORS = [
    "https://ghfast.top/" + RAW,
    "https://gh-proxy.com/" + RAW,
    "https://ghproxy.net/" + RAW,
    RAW,
]

for url in MIRRORS:
    print("trying", url, flush=True)
    try:
        req = urllib.request.Request(url, headers={"User-Agent": "Mozilla/5.0"})
        with urllib.request.urlopen(req, timeout=90) as r, open(TARGET, "wb") as f:
            total = 0
            while True:
                chunk = r.read(1024 * 256)
                if not chunk:
                    break
                f.write(chunk)
                total += len(chunk)
                if total > 120 * 1024 * 1024:
                    break
        size = os.path.getsize(TARGET)
        if size > 20 * 1024 * 1024:
            print("ok size=", size, flush=True)
            sys.exit(0)
        print("too small:", size, flush=True)
    except Exception as e:
        print("fail:", type(e).__name__, e, flush=True)

print("all mirrors failed", flush=True)
sys.exit(1)

import json
import urllib.request

BASE = "http://127.0.0.1:8080"


def req(method, path):
    r = urllib.request.Request(BASE + path, method=method)
    with urllib.request.urlopen(r, timeout=8) as resp:
        raw = resp.read()
        return json.loads(raw) if raw else None


state = req("GET", "/api/state")
total = 0
for t in ("losses", "sales", "strands", "batches"):
    for row in state.get(t, []):
        req("DELETE", "/api/items/%s/%d" % (t, row["id"]))
        total += 1

state = req("GET", "/api/state")
print("已删除 %d 条记录，当前剩余：%s" % (
    total, {k: len(v) for k, v in state.items()}))

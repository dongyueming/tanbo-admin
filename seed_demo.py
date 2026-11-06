import json
import urllib.request

BASE = "http://127.0.0.1:8080"


def req(method, path, body=None):
    data = json.dumps(body).encode("utf-8") if body is not None else None
    r = urllib.request.Request(
        BASE + path, data=data, method=method,
        headers={"Content-Type": "application/json"} if data else {}
    )
    with urllib.request.urlopen(r, timeout=8) as resp:
        raw = resp.read()
        return json.loads(raw) if raw else None


# wipe everything first
state = req("GET", "/api/state")
for t in ("losses", "sales", "strands", "batches"):
    for row in state.get(t, []):
        req("DELETE", "/api/items/%s/%d" % (t, row["id"]))

batches = [
    {"name": "包菜", "weight": 20, "pricePerJin": 1.8, "buyDate": "2026-11-01", "shelfDays": 4, "note": "菜市场东头老李"},
    {"name": "包菜", "weight": 18, "pricePerJin": 2.0, "buyDate": "2026-11-03", "shelfDays": 5, "note": "批发市场"},
    {"name": "包菜", "weight": 22, "pricePerJin": 1.6, "buyDate": "2026-11-05", "shelfDays": 6, "note": "早上新到的"},
]
bids = []
for b in batches:
    bids.append(req("POST", "/api/items/batches", b)["id"])

strands = [
    {"batchId": bids[0], "leaves": 8, "strands": 58, "date": "2026-11-01", "operator": "我"},
    {"batchId": bids[1], "leaves": 7, "strands": 52, "date": "2026-11-03", "operator": "我"},
    {"batchId": bids[2], "leaves": 9, "strands": 68, "date": "2026-11-05", "operator": "我"},
]
for s in strands:
    req("POST", "/api/items/strands", s)

sales = [
    {"batchId": bids[0], "strands": 26, "pricePerStrand": 2, "date": "2026-11-01", "channel": "零售"},
    {"batchId": bids[0], "strands": 18, "pricePerStrand": 2, "date": "2026-11-02", "channel": "零售"},
    {"batchId": bids[1], "strands": 30, "pricePerStrand": 2, "date": "2026-11-03", "channel": "零售"},
    {"batchId": bids[1], "strands": 14, "pricePerStrand": 2.5, "date": "2026-11-04", "channel": "零售"},
    {"batchId": bids[2], "strands": 40, "pricePerStrand": 2, "date": "2026-11-05", "channel": "零售"},
    {"batchId": bids[2], "strands": 10, "pricePerStrand": 1.8, "date": "2026-11-05", "channel": "批发"},
]
for s in sales:
    req("POST", "/api/items/sales", s)

losses = [
    {"batchId": bids[0], "strands": 9, "date": "2026-11-04", "reason": "临期", "note": "最后 9 串卖不动"},
    {"batchId": bids[2], "strands": 2, "date": "2026-11-05", "reason": "黄叶", "note": ""},
]
for l in losses:
    req("POST", "/api/items/losses", l)

final = req("GET", "/api/state")
print("seeded:", {k: len(v) for k, v in final.items()})

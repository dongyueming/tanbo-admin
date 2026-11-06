import http.cookiejar
import json
import urllib.error
import urllib.request

BASE = "http://127.0.0.1:8080"
PHONE = "17739465409"
PASS = "dongyueming123"

jar = http.cookiejar.CookieJar()
op = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(jar))


def post(path, body):
    req = urllib.request.Request(BASE + path, data=json.dumps(body).encode("utf-8"), method="POST")
    req.add_header("Content-Type", "application/json")
    try:
        r = op.open(req, timeout=10)
        return r.status, json.loads(r.read().decode("utf-8"))
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode("utf-8")


post("/api/login", {"phone": PHONE, "password": PASS})
state = json.loads(op.open(BASE + "/api/state", timeout=10).read().decode("utf-8"))
existing = {b.get("name") for b in state["batches"]}

PLAN = [
    {
        # 按「棵」进货的例子：30 棵 × 1 元，每棵剥 10 片共 300 片，2 片一串出 150 串
        # 单片成本 30/300 = 0.1 元，单串成本 30/150 = 0.2 元，卖 1 元/串 → 单串赚 0.8 元
        "batch": dict(category="vegetable", name="生菜", weight=30, unit="棵", pricePerJin=1,
                      buyDate="2026-11-04", shelfDays=3, note="按棵买的，一棵剥 10 片"),
        "strands": dict(leaves=300, strands=150, date="2026-11-04", operator="我"),
        "sales": [dict(strands=60, pricePerStrand=1, date="2026-11-04", channel="零售"),
                  dict(strands=60, pricePerStrand=1, date="2026-11-05", channel="零售")],
        "losses": [dict(strands=10, date="2026-11-05", reason="蔫了", note="放两天没卖完")]
    },
    {
        "batch": dict(category="meat", name="牛肉", weight=15, unit="斤", pricePerJin=38,
                      buyDate="2026-11-02", shelfDays=2, note="牛腱子，切片穿串"),
        "strands": dict(leaves=0, strands=90, date="2026-11-02", operator="我"),
        "sales": [dict(strands=40, pricePerStrand=8, date="2026-11-02", channel="零售"),
                  dict(strands=35, pricePerStrand=8, date="2026-11-03", channel="零售")],
        "losses": [dict(strands=6, date="2026-11-04", reason="变质", note="隔夜没卖完")]
    },
    {
        "batch": dict(category="ball", name="鱼丸", weight=6, unit="包", pricePerJin=12,
                      buyDate="2026-11-02", shelfDays=30, note="冷冻品，每包约 30 颗"),
        "strands": dict(leaves=0, strands=180, date="2026-11-02", operator="我"),
        "sales": [dict(strands=90, pricePerStrand=1.5, date="2026-11-03", channel="零售"),
                  dict(strands=60, pricePerStrand=1.5, date="2026-11-04", channel="零售")],
        "losses": []
    },
    {
        "batch": dict(category="sausage", name="火腿肠", weight=4, unit="包", pricePerJin=15,
                      buyDate="2026-11-04", shelfDays=15, note="每包 30 根，切段穿串"),
        "strands": dict(leaves=0, strands=120, date="2026-11-04", operator="我"),
        "sales": [dict(strands=60, pricePerStrand=1.5, date="2026-11-04", channel="零售"),
                  dict(strands=40, pricePerStrand=1.5, date="2026-11-05", channel="零售")],
        "losses": []
    },
    {
        "batch": dict(category="base", name="牛油底料", weight=5, unit="袋", pricePerJin=18,
                      buyDate="2026-11-01", shelfDays=60, note="锅底用，按份算不穿串"),
        "strands": dict(leaves=0, strands=100, date="2026-11-01", operator="我"),
        "sales": [dict(strands=40, pricePerStrand=3, date="2026-11-02", channel="零售"),
                  dict(strands=20, pricePerStrand=3, date="2026-11-04", channel="零售")],
        "losses": [dict(strands=5, date="2026-11-05", reason="开封变质", note="放太久结块")]
    }
]

for item in PLAN:
    b = item["batch"]
    if b["name"] in existing:
        print("跳过（已存在）:", b["name"])
        continue

    st, res = post("/api/items/batches", b)
    if st not in (200, 201):
        print("批次失败", b["name"], st, res)
        continue
    bid = res["id"]
    print("批次 #%s %s 已建" % (bid, b["name"]))

    s = dict(item["strands"])
    s["batchId"] = bid
    st, _ = post("/api/items/strands", s)
    print("  穿串记录:", st)

    for sale in item["sales"]:
        row = dict(sale)
        row["batchId"] = bid
        st, _ = post("/api/items/sales", row)
        print("  销售记录:", st)

    for loss in item["losses"]:
        row = dict(loss)
        row["batchId"] = bid
        st, _ = post("/api/items/losses", row)
        print("  报损记录:", st)

print("\n完成。")
state = json.loads(op.open(BASE + "/api/state", timeout=10).read().decode("utf-8"))
print("当前统计：批次 %d / 穿串 %d / 销售 %d / 报损 %d" % (
    len(state["batches"]), len(state["strands"]), len(state["sales"]), len(state["losses"])))

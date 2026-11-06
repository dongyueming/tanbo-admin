import http.cookiejar
import json
import urllib.request

BASE = "http://127.0.0.1:8080"

jar = http.cookiejar.CookieJar()
op = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(jar))


def req(method, path, body=None):
    data = json.dumps(body).encode("utf-8") if body is not None else None
    r = urllib.request.Request(BASE + path, data=data, method=method)
    if data:
        r.add_header("Content-Type", "application/json")
    res = op.open(r, timeout=10)
    return res.status, res.read().decode("utf-8")


# 登录
req("POST", "/api/login", {"phone": "17739465409", "password": "dongyueming123"})

# 四个品类的参考批次
batches = [
    {
        "category": "soup",
        "name": "30斤锅底汤",
        "weight": 25,
        "unit": "斤",
        "pricePerJin": 1.64,
        "buyDate": "2026-11-05",
        "shelfDays": 3,
        "note": "含牛骨2根+大料包+汤底调料，5天换一次"
    },
    {
        "category": "secret",
        "name": "秘制酱",
        "weight": 12,
        "unit": "斤",
        "pricePerJin": 6.58,
        "buyDate": "2026-11-05",
        "shelfDays": 15,
        "note": "菜籽油6斤+芝麻香辣酱1桶+调料"
    },
    {
        "category": "chili",
        "name": "辣酱（辣油）",
        "weight": 3,
        "unit": "斤",
        "pricePerJin": 9.67,
        "buyDate": "2026-11-05",
        "shelfDays": 15,
        "note": "菜籽油3斤+花椒辣椒花生芝麻"
    },
    {
        "category": "sweet",
        "name": "甜酱",
        "weight": 6,
        "unit": "斤",
        "pricePerJin": 4.83,
        "buyDate": "2026-11-05",
        "shelfDays": 15,
        "note": "甜面酱1桶+白糖500g+调料"
    }
]

for b in batches:
    st, body = req("POST", "/api/items/batches", b)
    print("录入 %s -> %s %s" % (b["name"], st, body))

# 查看总数
state = json.loads(op.open(BASE + "/api/state", timeout=10).read().decode("utf-8"))
print("")
print("当前批次总数:", len(state["batches"]))
print("最新4批:")
for b in state["batches"][-4:]:
    print("  #%d %s %s %.1f%s 单价%.2f" % (b["id"], b["category"], b["name"], b["weight"], b["unit"], b["pricePerJin"]))

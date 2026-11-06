import http.cookiejar
import json
import urllib.error
import urllib.request

BASE = "http://127.0.0.1:8080"

jar = http.cookiejar.CookieJar()
op = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(jar))


def req(method, path, body=None):
    data = json.dumps(body).encode("utf-8") if body is not None else None
    r = urllib.request.Request(BASE + path, data=data, method=method)
    if data:
        r.add_header("Content-Type", "application/json")
    try:
        res = op.open(r, timeout=10)
        return res.status, res.read().decode("utf-8")
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode("utf-8")


req("POST", "/api/login", {"phone": "17739465409", "password": "dongyueming123"})

state = json.loads(op.open(BASE + "/api/state", timeout=10).read().decode("utf-8"))

# 早期演示数据里 leaves 记的是「剥掉的烂叶」，语义和现在的「总共用了几片」不一致，
# 按每串 3 片重新折算，让单片成本说得通
FIX = {1: 3, 2: 3, 3: 3}

for s in state["strands"]:
    bid = s.get("batchId")
    if bid in FIX and s.get("leaves", 0) < s.get("strands", 0):
        new_leaves = int(s["strands"] * FIX[bid])
        st, _ = req("PUT", "/api/items/strands/%d" % s["id"], dict(
            batchId=bid, leaves=new_leaves, strands=s["strands"],
            date=s.get("date"), operator=s.get("operator"), note=s.get("note")
        ))
        print("穿串 #%d (批次%d): leaves %s -> %d  [%s]" % (s["id"], bid, s.get("leaves"), new_leaves, st))

state = json.loads(op.open(BASE + "/api/state", timeout=10).read().decode("utf-8"))
print("")
print("修正后的穿串记录：")
for s in state["strands"]:
    print("  #%d 批次%d 片数%s 串数%s" % (s["id"], s.get("batchId"), s.get("leaves"), s.get("strands")))

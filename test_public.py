import http.cookiejar
import json
import urllib.error
import urllib.request

BASE = "https://foam-introduction-fellowship-deer.trycloudflare.com"
PHONE = "17739465409"
PASS = "dongyueming123"

results = []


def call(method, path, body=None, opener=None):
    data = json.dumps(body).encode("utf-8") if body is not None else None
    req = urllib.request.Request(BASE + path, data=data, method=method)
    if data:
        req.add_header("Content-Type", "application/json")
    o = opener or urllib.request.build_opener()
    try:
        r = o.open(req, timeout=25)
        return r.status, r.read()[:200]
    except urllib.error.HTTPError as e:
        return e.code, e.read()[:200]


def note(label, expect, got):
    results.append((label, expect, got))


s, body = call("GET", "/")
note("公网打开首页", 200, s)
note("  首页含中文标题", True, "小摊串串经营台账".encode("utf-8") in body or "串" in body.decode("utf-8", "replace"))

s, _ = call("GET", "/api/state")
note("公网未登录读数据", 401, s)

jar = http.cookiejar.CookieJar()
opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(jar))
s, _ = call("POST", "/api/login", {"phone": PHONE, "password": PASS}, opener)
note("公网登录", 200, s)
note("  拿到会话 cookie", True, len(jar) > 0)

s, _ = call("GET", "/api/state", opener=opener)
note("公网登录后读数据", 200, s)

print("%-22s %-8s %s" % ("用例", "期望", "实际"))
print("-" * 46)
for label, expect, got in results:
    flag = "OK " if str(expect) == str(got) else "FAIL"
    print("%-22s %-8s %-8s %s" % (label, expect, got, flag))

bad = [r for r in results if str(r[1]) != str(r[2])]
print("-" * 46)
print("全部通过" if not bad else "有 %d 项不符合预期" % len(bad))

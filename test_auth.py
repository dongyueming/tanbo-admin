import http.cookiejar
import json
import urllib.error
import urllib.request

BASE = "http://127.0.0.1:8080"
OK = "17739465409"
OK2 = "18768335998"
BADPASS = "17739465409"  # 正确手机号 + 错误密码
OUTSIDER = "13800000000"

jar = http.cookiejar.CookieJar()
opener = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(jar))
results = []


def call(method, path, body=None, use_opener=True):
    data = json.dumps(body).encode("utf-8") if body is not None else None
    req = urllib.request.Request(BASE + path, data=data, method=method)
    if data:
        req.add_header("Content-Type", "application/json")
    try:
        r = (opener if use_opener else urllib.request.build_opener()).open(req, timeout=10)
        return r.status, r.read().decode("utf-8", "replace")
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode("utf-8", "replace")


def note(label, expect, got):
    results.append((label, expect, got))


ACCOUNTS = {OK: "dongyueming123", OK2: "zzzmmmkkk323232"}

# 1. 未登录访问数据
s, _ = call("GET", "/api/state", use_opener=False)
note("未登录读数据", 401, s)

# 2. 未登录读 /api/me
s, _ = call("GET", "/api/me", use_opener=False)
note("未登录查身份", 401, s)

# 3. 正确手机号 + 正确密码
s, b = call("POST", "/api/login", {"phone": OK, "password": ACCOUNTS[OK]})
note("账号1登录", 200, s)
note("  返回手机号", OK, json.loads(b).get("phone") if s == 200 else "-")

# 4. 带 cookie 读数据
s, b = call("GET", "/api/state")
note("登录后读数据", 200, s)
if s == 200:
    d = json.loads(b)
    n = len(d.get("batches", []))
    note("  批次数 > 0", True, n > 0)

# 5. 正确手机号 + 错误密码
s, _ = call("POST", "/api/login", {"phone": OK, "password": "wrongpass"}, use_opener=False)
note("密码错误", 401, s)

# 6. 未登记的手机号
s, _ = call("POST", "/api/login", {"phone": OUTSIDER, "password": "anything"}, use_opener=False)
note("陌生手机号", 403, s)

# 7. 第二个账号也能登录
jar2 = http.cookiejar.CookieJar()
opener2 = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(jar2))
old = opener
opener = opener2
s, _ = call("POST", "/api/login", {"phone": OK2, "password": ACCOUNTS[OK2]})
note("账号2登录", 200, s)
s, _ = call("GET", "/api/state")
note("  账号2读数据", 200, s)
opener = old

# 8. 退出登录后失效
s, _ = call("POST", "/api/logout")
note("退出登录", 200, s)
jar.clear()
s, _ = call("GET", "/api/state")
note("退出后读数据", 401, s)

print("%-20s %-8s %s" % ("用例", "期望", "实际"))
print("-" * 44)
for label, expect, got in results:
    flag = "OK " if str(expect) == str(got) else "FAIL"
    print("%-20s %-8s %-8s %s" % (label, expect, got, flag))

bad = [r for r in results if str(r[1]) != str(r[2])]
print("-" * 44)
print("全部通过" if not bad else "有 %d 项不符合预期" % len(bad))

import http.cookiejar, json, urllib.request, random

BASE = 'http://127.0.0.1:8080'
jar = http.cookiejar.CookieJar()
op = urllib.request.build_opener(urllib.request.HTTPCookieProcessor(jar))

def req(m, p, b=None):
    d = json.dumps(b).encode() if b else None
    r = urllib.request.Request(BASE + p, data=d, method=m)
    if d:
        r.add_header('Content-Type', 'application/json')
    res = op.open(r, timeout=10)
    return res.status, res.read().decode()

# 登录
req('POST', '/api/login', {'phone': '17739465409', 'password': 'dongyueming123'})

# 先清空旧数据
state = json.loads(op.open(BASE + '/api/state', timeout=10).read().decode())
for item_type in ['batches', 'strands', 'sales', 'losses', 'expenses']:
    for item in state.get(item_type, []):
        try:
            req('DELETE', f'/api/items/{item_type}/{item["id"]}')
        except:
            pass

print('旧数据已清空')

# 灌入进货批次（蔬菜/肉/丸子/肠/底料）
batches = [
    # 蔬菜
    {'category': 'vegetable', 'name': '包菜', 'weight': 20, 'unit': '斤', 'pricePerJin': 1.8, 'buyDate': '2026-11-01', 'shelfDays': 4, 'note': '菜市场东头老李'},
    {'category': 'vegetable', 'name': '生菜', 'weight': 15, 'unit': '斤', 'pricePerJin': 2.5, 'buyDate': '2026-11-02', 'shelfDays': 3, 'note': ''},
    {'category': 'vegetable', 'name': '土豆', 'weight': 30, 'unit': '斤', 'pricePerJin': 1.2, 'buyDate': '2026-11-03', 'shelfDays': 7, 'note': ''},
    # 肉类
    {'category': 'meat', 'name': '牛肉', 'weight': 10, 'unit': '斤', 'pricePerJin': 38, 'buyDate': '2026-11-01', 'shelfDays': 2, 'note': '牛里脊'},
    {'category': 'meat', 'name': '五花肉', 'weight': 8, 'unit': '斤', 'pricePerJin': 22, 'buyDate': '2026-11-02', 'shelfDays': 2, 'note': ''},
    # 丸子
    {'category': 'ball', 'name': '鱼丸', 'weight': 5, 'unit': '包', 'pricePerJin': 12, 'buyDate': '2026-11-01', 'shelfDays': 30, 'note': ''},
    {'category': 'ball', 'name': '牛肉丸', 'weight': 4, 'unit': '包', 'pricePerJin': 15, 'buyDate': '2026-11-02', 'shelfDays': 30, 'note': ''},
    # 肠类
    {'category': 'sausage', 'name': '火腿肠', 'weight': 3, 'unit': '包', 'pricePerJin': 15, 'buyDate': '2026-11-01', 'shelfDays': 15, 'note': ''},
    # 底料
    {'category': 'base', 'name': '牛油底料', 'weight': 2, 'unit': '袋', 'pricePerJin': 18, 'buyDate': '2026-11-01', 'shelfDays': 60, 'note': ''},
]

batch_ids = []
for b in batches:
    st, res = req('POST', '/api/items/batches', b)
    if st == 201:
        batch_ids.append(json.loads(res)['id'])
        print(f"  进货: {b['name']} {b['weight']}{b['unit']} -> 第{json.loads(res)['id']}批")

# 灌入穿串记录
strands = [
    {'batchId': batch_ids[0], 'leaves': 200, 'strands': 66, 'date': '2026-11-01', 'operator': '老板娘', 'note': ''},
    {'batchId': batch_ids[1], 'leaves': 150, 'strands': 75, 'date': '2026-11-02', 'operator': '老板娘', 'note': ''},
    {'batchId': batch_ids[3], 'leaves': 0, 'strands': 80, 'date': '2026-11-01', 'operator': '老板娘', 'note': ''},
    {'batchId': batch_ids[4], 'leaves': 0, 'strands': 60, 'date': '2026-11-02', 'operator': '老板娘', 'note': ''},
]
for s in strands:
    st, res = req('POST', '/api/items/strands', s)
    if st == 201:
        print(f"  穿串: 第{s['batchId']}批 -> {s['strands']}串")

# 灌入销售记录
sales = [
    {'batchId': batch_ids[0], 'strands': 50, 'pricePerStrand': 1.5, 'date': '2026-11-01', 'channel': '零售', 'note': ''},
    {'batchId': batch_ids[0], 'strands': 30, 'pricePerStrand': 1.5, 'date': '2026-11-02', 'channel': '零售', 'note': ''},
    {'batchId': batch_ids[1], 'strands': 40, 'pricePerStrand': 1.0, 'date': '2026-11-02', 'channel': '零售', 'note': ''},
    {'batchId': batch_ids[3], 'strands': 60, 'pricePerStrand': 3.0, 'date': '2026-11-01', 'channel': '零售', 'note': ''},
    {'batchId': batch_ids[4], 'strands': 40, 'pricePerStrand': 2.5, 'date': '2026-11-02', 'channel': '零售', 'note': ''},
    {'batchId': batch_ids[5], 'strands': 80, 'pricePerStrand': 1.5, 'date': '2026-11-01', 'channel': '零售', 'note': ''},
    {'batchId': batch_ids[6], 'strands': 60, 'pricePerStrand': 2.0, 'date': '2026-11-02', 'channel': '零售', 'note': ''},
    {'batchId': batch_ids[7], 'strands': 100, 'pricePerStrand': 1.0, 'date': '2026-11-01', 'channel': '零售', 'note': ''},
]
for s in sales:
    st, res = req('POST', '/api/items/sales', s)
    if st == 201:
        print(f"  销售: 第{s['batchId']}批 -> {s['strands']}串 × {s['pricePerStrand']}元")

# 灌入损耗记录
losses = [
    {'batchId': batch_ids[0], 'strands': 6, 'date': '2026-11-02', 'reason': '黄叶', 'note': '烂叶'},
    {'batchId': batch_ids[1], 'strands': 5, 'date': '2026-11-03', 'reason': '临期', 'note': '雨天人少'},
]
for l in losses:
    st, res = req('POST', '/api/items/losses', l)
    if st == 201:
        print(f"  损耗: 第{l['batchId']}批 -> {l['strands']}串 ({l['reason']})")

# 灌入其他支出记录
expenses = [
    {'cat': 'labor', 'name': '请阿姨帮忙穿串', 'cost': 50, 'date': '2026-11-01', 'note': ''},
    {'cat': 'gas', 'name': '煤气一罐', 'cost': 30, 'date': '2026-11-01', 'note': ''},
    {'cat': 'rent', 'name': '摊位费', 'cost': 20, 'date': '2026-11-01', 'note': '每天'},
    {'cat': 'pack', 'name': '签子+袋子+手套', 'cost': 15, 'date': '2026-11-01', 'note': ''},
    {'cat': 'labor', 'name': '请阿姨帮忙穿串', 'cost': 50, 'date': '2026-11-02', 'note': ''},
    {'cat': 'gas', 'name': '煤气一罐', 'cost': 30, 'date': '2026-11-02', 'note': ''},
]
for e in expenses:
    st, res = req('POST', '/api/items/expenses', e)
    if st == 201:
        print(f"  支出: {e['name']} {e['cost']}元")

# 灌入四个酱料
sauces = [
    {'category': 'soup', 'name': '30斤锅底汤', 'weight': 25, 'unit': '斤', 'pricePerJin': 1.64, 'buyDate': '2026-11-01', 'shelfDays': 3, 'note': '牛骨2根+大料包+汤底调料'},
    {'category': 'secret', 'name': '秘制酱', 'weight': 12, 'unit': '斤', 'pricePerJin': 6.58, 'buyDate': '2026-11-01', 'shelfDays': 15, 'note': '菜籽油6斤+芝麻香辣酱1桶'},
    {'category': 'chili', 'name': '辣酱（辣油）', 'weight': 3, 'unit': '斤', 'pricePerJin': 9.67, 'buyDate': '2026-11-01', 'shelfDays': 15, 'note': '菜籽油3斤+花椒辣椒'},
    {'category': 'sweet', 'name': '甜酱', 'weight': 6, 'unit': '斤', 'pricePerJin': 4.83, 'buyDate': '2026-11-01', 'shelfDays': 15, 'note': '甜面酱1桶+白糖500g'},
]
for s in sauces:
    st, res = req('POST', '/api/items/batches', s)
    if st == 201:
        print(f"  酱料: {s['name']} -> 第{json.loads(res)['id']}批")

print('\n数据灌入完成！')
state = json.loads(op.open(BASE + '/api/state', timeout=10).read().decode())
print(f"批次: {len(state['batches'])} 条")
print(f"穿串: {len(state['strands'])} 条")
print(f"销售: {len(state['sales'])} 条")
print(f"损耗: {len(state['losses'])} 条")
print(f"支出: {len(state['expenses'])} 条")

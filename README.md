# 小摊串串经营台账

一个给街边小摊用的后台管理系统：一颗包菜从进货、剥叶、穿串、卖串到结利润，全程一本账，自动算毛利、出品率、保鲜倒计时。

- 前端：Vue 3 + Vite + ECharts
- 后端：C（C99，手写 HTTP 服务器，不依赖任何第三方库）
- 存储：本地 JSON 文件（`data/`），复制走就能备份
- 前后端同一个进程对外提供服务，部署只需要「一个二进制 + 一个 dist 目录」

## 管什么

| 模块 | 记什么 |
| --- | --- |
| 包菜批次 | 品类、斤数、每斤进价、进货日、保鲜天数、到期日 |
| 剥叶穿串 | 哪一批、剥几片叶、出几串、操作人 |
| 销售记账 | 哪一批、卖几串、每串多少钱、渠道（零售/批发/团购/自用） |
| 损耗报损 | 哪一批、坏了几串、原因（黄叶/临期/破损/自用） |
| 仪表盘 | 今日收入、今日毛利、在手库存、临期提醒、近 7 天走势 |
| 利润报表 | 批次毛利排行、收入构成、出品率对比、明细表 |

## 利润怎么算

```
原料成本   = 斤数 × 每斤进价
销售收入   = 售出串数 × 每串售价
毛利       = 销售收入 − 原料成本
毛利率     = 毛利 ÷ 销售收入 × 100%
出品率     = 出品串数 ÷ 斤数          （一斤能穿几串，越高越赚）
单串成本   = 原料成本 ÷ 出品串数       （定价的底线）
```

损耗不额外扣成本——没卖出去的串本来就没产生收入，报损只用于算损耗率、看库存和找原因。

## 目录

```
tanbo-admin/
├─ src/                  Vue 源码
│  ├─ views/             六个页面
│  ├─ components/        ECharts 封装
│  ├─ stats.js           所有统计与利润计算
│  └─ api.js             接口封装
├─ server/               C 后端源码
│  ├─ main.c             HTTP 服务、路由、静态文件
│  ├─ minijson.c/.h      JSON 解析与序列化
│  ├─ build.ps1          Windows 编译
│  └─ build.sh           Linux 编译
├─ dist/                 npm run build 生成的前端产物
├─ data/                 运行时数据（batches/strands/sales/losses.json）
├─ tanbo-server.exe      Windows 可执行
└─ start.ps1             一键启动
```

## 本机运行（Windows）

```powershell
cd tanbo-admin

# 1. 编译后端（MinGW gcc）
.\server\build.ps1 -Gcc "C:\Users\Administrator\tools\mingw64\bin\gcc.exe"

# 2. 构建前端
npm install
npm run build

# 3. 启动
.\start.ps1 -Port 8080
```

浏览器打开 `http://127.0.0.1:8080`。

## 本机常驻（已配置好）

- 服务监听 `0.0.0.0:8080`，同一 WiFi 下的手机可以直接打开 `http://192.168.110.216:8080`
- 开机自启：`%APPDATA%\Microsoft\Windows\Start Menu\Programs\Startup\TanboAdmin.vbs`，登录 Windows 时静默拉起，不弹窗
- 手机扫码入口：`手机扫码访问.png`
- 运行日志：`server.out.log` / `server.err.log`

注意：局域网 IP 是路由器分配的，重启路由器后可能变化。IP 变了就重新生成二维码，或者到路由器里给这台机器绑定固定 IP。

电脑关机或睡眠后服务就停了，手机也就打不开——要 7×24 小时可用，得搬到云服务器（见下一节）。

## 品类与计价单位

现在不只包菜，五个品类都支持：

| 品类 | 默认单位 | 默认保鲜 | 怎么记 |
| --- | --- | --- | --- |
| 蔬菜 | 斤 | 4 天 | 包菜、生菜、金针菇、土豆…按斤进货，剥叶/切片后穿串 |
| 肉类 | 斤 | 2 天 | 牛肉、羊肉、鸡肉、鸡翅…按斤进货，切片腌好穿串 |
| 丸子 | 包 | 30 天 | 鱼丸、虾滑、撒尿牛丸…冷冻品按包进货，一颗一串 |
| 肠类 | 包 | 15 天 | 火腿肠、热狗肠…按包进货，切段或整根穿串 |
| 底料 | 袋 | 60 天 | 火锅底料、蘸料…按袋进货，按份用，不穿串 |

计价单位可以改（斤 / 包 / 袋 / 瓶 / 份 / 箱 / 个），但成本永远是一个公式：

```
原料成本 = 数量 × 单价
```

记「牛肉丸」这种跨界的东西，品类选「丸子」，品名写清楚就行——品类只影响默认单位、保鲜天数和统计分组。

统计上有个限制要知道：**斤和包不能相加**，所以「合计」只汇总钱和串数，不汇总进货量；想看进货量得按品类分开看。报表页有「各品类盈亏」表，成本、收入、毛利、毛利率、占比都在里面。

## 单片成本 → 单串利润

系统里有两个利润口径，别混着看：

**一、单位经济（一串赚多少）—— 用来定价**

```
单片成本 = 原料成本 ÷ 总片数
单串成本 = 原料成本 ÷ 出品串数   （等价于 单片成本 × 每串用几片）
单串利润 = 每串售价 − 单串成本
```

**二、整批毛利（这批货总共赚多少）—— 用来盘账**

```
毛利 = 销售收入 − 原料成本
```

拿系统里第 8 批「生菜」的真实数据走一遍：

- 30 棵 × 1 元 = **30 元**成本
- 剥出 300 片，2 片穿一串，出品 **150 串**
- 单片成本 30 ÷ 300 = **0.10 元**，每串 2 片 → 单串成本 30 ÷ 150 = **0.20 元**
- 卖 1 元/串 → 单串利润 = 1.00 − 0.20 = **0.80 元**

**这里没有一处是写死的。** 棵数、单价、片数、每串几片、售价，全是每次录入时填的，所有比率当场算出来。这棵菜剥 8 片、下一棵剥 12 片、这周卖 1 块下周卖 1 块 2，系统都照实反映——而且老批次用的还是老数据，不会因为改了今天的价格就把上个月的账跟着变。

叶片数不填也行：单串成本和单串利润照样算得出来（走「成本 ÷ 串数」这条），只是表格里「单片成本」「每串片数」两栏显示「-」。肉、丸子、肠、底料这些本来就没有「片」的概念，填 0 就行，不影响任何计算。

## 登录

系统要求手机号 + 密码登录，只有登记在册的手机号能进。

| 手机号 | 密码 |
| --- | --- |
| `17739465409` | `dongyueming123` |
| `18768335998` | `zzzmmmkkk323232` |

账号存放在 `data/accounts.txt`，一行一个，格式 `手机号,密码`：

```
17739465409,dongyueming123
18768335998,zzzmmmkkk323232
```

改密码就是改这个文件（改前面那行逗号后面的部分），然后重启服务。加人也一样，加一行再重启。没有这个文件时，所有人都能进——所以别删它。

登录成功后服务端会发一个 7 天有效的会话 cookie，期间不用重复登录。会话存在内存里，服务一重启所有人都要重新登录。

注意：密码是明文比对，局域网和 Cloudflare 隧道都走 HTTP/HTTPS 传输，但服务器本身不加密存储账号文件。放在公网长期用，建议前面套一层 HTTPS。

## 公网访问（Cloudflare 临时隧道）

在不想买服务器的情况下，用 Cloudflare 免费临时隧道把本机服务暴露到公网：

```powershell
.\tools\start-tunnel.ps1
```

脚本会启动隧道并打印当次分配到的 `https://xxxx.trycloudflare.com` 地址。

**这次跑通的地址**（重启隧道后会失效）：

```
https://foam-introduction-fellowship-deer.trycloudflare.com
```

账号 `17739465409` / `18768335998`，密码见上一节。

限制要清楚：

- 电脑关机、睡眠、断网，外面就打不开
- 每次重启隧道地址都会变（免费版不支持固定域名），要固定就注册 Cloudflare 账号建命名隧道
- 免费隧道没有可用性承诺，只适合自用，不适合当正式生产环境
- 关掉公网入口：`Get-Process cloudflared | Stop-Process`

## 部署到 Linux 服务器

服务器上只要有 `gcc`，不需要 Node。

```bash
# 1. 上传 server/ 和 dist/ 两个目录，以及 data/（如果已有数据）
scp -r server dist user@your-server:/opt/tanbo/

# 2. 编译
cd /opt/tanbo
sh server/build.sh            # 产物 /opt/tanbo/tanbo-server

# 3. 先手工跑一下
./tanbo-server --port 8080 --data /opt/tanbo/data --static /opt/tanbo/dist
```

做成常驻服务（systemd），`/etc/systemd/system/tanbo.service`：

```ini
[Unit]
Description=Tanbo Admin
After=network.target

[Service]
Type=simple
WorkingDirectory=/opt/tanbo
ExecStart=/opt/tanbo/tanbo-server --port 8080 --data /opt/tanbo/data --static /opt/tanbo/dist
Restart=always
RestartSec=3
User=www-data

[Install]
WantedBy=multi-user.target
```

```bash
sudo systemctl daemon-reload
sudo systemctl enable --now tanbo
sudo systemctl status tanbo
```

要绑域名走 80/443，前面加一层 Nginx 反代到 `127.0.0.1:8080`：

```nginx
server {
    listen 80;
    server_name tanbo.example.com;
    location / {
        proxy_pass http://127.0.0.1:8080;
        proxy_set_header Host $host;
        proxy_set_header X-Real-IP $remote_addr;
    }
}
```

用 `certbot --nginx` 可以顺手把 HTTPS 配上。

## 接口

| 方法 | 路径 | 说明 |
| --- | --- | --- |
| GET | `/api/health` | 健康检查 |
| GET | `/api/state` | 一次性拉全部数据 |
| POST | `/api/items/{type}` | 新增，返回带 id 的对象 |
| PUT | `/api/items/{type}/{id}` | 更新（只覆盖提交的字段） |
| DELETE | `/api/items/{type}/{id}` | 删除 |

`type` 取值：`batches`、`strands`、`sales`、`losses`。

## 数据与备份

数据在 `data/` 下四个 JSON 文件，每次写入都是「先写临时文件再替换」，断电不会写坏。

备份就是复制整个 `data/` 目录：

```bash
tar czf tanbo-$(date +%F).tar.gz /opt/tanbo/data
```

## 注意

- 数据是明文 JSON，没有登录鉴权。放到公网请务必在 Nginx 层加 Basic Auth 或只在内网使用。
- 服务是单线程短连接模型：日常几个人的小摊完全够用，不适合高并发。
- 端口默认 8080，可用 `--port` 或环境变量 `PORT` 改。

# 启动公网隧道，并打印本次的随机网址
# 用法：在 PowerShell 里运行  .\tools\start-tunnel.ps1
# 注意：每次重启这个脚本，网址都会变（Cloudflare 免费临时隧道不支持固定域名）

$proj = Split-Path -Parent $PSScriptRoot
$exe = Join-Path $PSScriptRoot "cloudflared.exe"
$log = Join-Path $PSScriptRoot "tunnel.err.log"

$svc = Get-Process tanbo-server -ErrorAction SilentlyContinue
if (-not $svc) {
    Write-Output "本地服务没在跑，先启动它："
    Start-Process -FilePath (Join-Path $proj "tanbo-server.exe") `
        -ArgumentList "--port","8080","--data",(Join-Path $proj "data"),"--static",(Join-Path $proj "dist") `
        -WindowStyle Hidden
    Start-Sleep -Seconds 3
}

if (-not (Get-Process cloudflared -ErrorAction SilentlyContinue)) {
    Start-Process -FilePath $exe `
        -ArgumentList "tunnel","--url","http://127.0.0.1:8080","--no-autoupdate" `
        -WindowStyle Hidden `
        -RedirectStandardOutput (Join-Path $PSScriptRoot "tunnel.out.log") `
        -RedirectStandardError $log
    Start-Sleep -Seconds 12
}

$hit = Select-String -LiteralPath $log -Pattern "https://[a-z0-9-]+\.trycloudflare\.com" | Select-Object -Last 1
if ($hit) {
    Write-Output ""
    Write-Output ("公网地址: " + $hit.Matches[0].Value)
    Write-Output "登录方式: 手机号 + 密码（账号见 data\accounts.txt）"
    Write-Output ""
    Write-Output "关掉公网入口: Get-Process cloudflared | Stop-Process"
} else {
    Write-Output ("没拿到地址，看日志: " + $log)
}

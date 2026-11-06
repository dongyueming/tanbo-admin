param(
  [int]$Port = 8080
)
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location -LiteralPath $root

if (-not (Test-Path -LiteralPath (Join-Path $root "tanbo-server.exe"))) {
  Write-Host "还没有编译，先跑 server\build.ps1"
  exit 1
}

if (-not (Test-Path -LiteralPath (Join-Path $root "dist\index.html"))) {
  Write-Host "还没有前端产物，先跑 npm run build"
  exit 1
}

Write-Host "启动：http://127.0.0.1:$Port"
& (Join-Path $root "tanbo-server.exe") --port $Port --data (Join-Path $root "data") --static (Join-Path $root "dist")

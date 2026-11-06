param(
  [string]$Gcc = "",
  [string]$Out = ""
)

$ErrorActionPreference = "Stop"
$here = Split-Path -Parent $MyInvocation.MyCommand.Path
$root = Split-Path -Parent $here

if (-not $Gcc) {
  $candidates = @(
    "C:\Users\Administrator\tools\mingw64\bin\gcc.exe",
    "C:\mingw64\bin\gcc.exe"
  )
  foreach ($c in $candidates) {
    if (Test-Path -LiteralPath $c) { $Gcc = $c; break }
  }
  if (-not $Gcc) {
    $cmd = Get-Command gcc -ErrorAction SilentlyContinue
    if ($cmd) { $Gcc = $cmd.Source }
  }
}

if (-not $Gcc) {
  throw "找不到 gcc。请用 -Gcc 指定路径，或把 MinGW 的 bin 目录加入 PATH。"
}

if (-not $Out) {
  $Out = Join-Path $root "tanbo-server.exe"
}

Write-Host "gcc  : $Gcc"
Write-Host "out  : $Out"

& $Gcc -O2 -std=c99 -Wall -o $Out `
  (Join-Path $here "main.c") `
  (Join-Path $here "minijson.c") `
  -lws2_32

if ($LASTEXITCODE -ne 0) { throw "编译失败" }
Write-Host "编译完成: $Out"

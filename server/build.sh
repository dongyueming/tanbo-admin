#!/bin/sh
set -e
HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(dirname "$HERE")
OUT="${1:-$ROOT/tanbo-server}"

echo "gcc  : $(command -v gcc)"
echo "out  : $OUT"

gcc -O2 -std=c99 -Wall -o "$OUT" \
  "$HERE/main.c" \
  "$HERE/minijson.c"

echo "编译完成: $OUT"

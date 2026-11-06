#!/bin/sh
# 一键部署到 Linux 服务器
# 用法：sh server/deploy.sh <IP或域名> [用户名] [SSH端口] [远程目录]
# 示例：sh server/deploy.sh 47.98.12.34 root 22 /opt/tanbo
set -e

HOST="$1"
REMOTE_USER="${2:-root}"
SSH_PORT="${3:-22}"
REMOTE_DIR="${4:-/opt/tanbo}"

if [ -z "$HOST" ]; then
  echo "用法: sh server/deploy.sh <IP或域名> [用户名=root] [SSH端口=22] [远程目录=/opt/tanbo]"
  exit 1
fi

HERE=$(cd "$(dirname "$0")" && pwd)
ROOT=$(dirname "$HERE")
TAR=/tmp/tanbo-deploy.tar.gz

echo "[1/4] 打包 server/ dist/ data/"
rm -f "$TAR"
if [ -d "$ROOT/data" ]; then
  tar czf "$TAR" -C "$ROOT" server dist data
else
  tar czf "$TAR" -C "$ROOT" server dist
fi
echo "      包大小 $(du -h "$TAR" | cut -f1)"

echo "[2/4] 上传到 $REMOTE_USER@$HOST:$SSH_PORT"
scp -P "$SSH_PORT" "$TAR" "$REMOTE_USER@$HOST:/tmp/tanbo-deploy.tar.gz"

echo "[3/4] 解包并编译"
ssh -p "$SSH_PORT" "$REMOTE_USER@$HOST" "set -e
sudo mkdir -p $REMOTE_DIR
sudo tar xzf /tmp/tanbo-deploy.tar.gz -C $REMOTE_DIR
cd $REMOTE_DIR
command -v gcc >/dev/null || { echo '服务器上没有 gcc，执行: sudo apt install -y build-essential'; exit 1; }
sh server/build.sh $REMOTE_DIR/tanbo-server
mkdir -p $REMOTE_DIR/data
"

echo "[4/4] 启动或重启"
ssh -p "$SSH_PORT" "$REMOTE_USER@$HOST" "set -e
cd $REMOTE_DIR
if systemctl is-active --quiet tanbo 2>/dev/null; then
  sudo systemctl restart tanbo
  echo '已通过 systemd 重启'
else
  if [ -f tanbo.pid ] && kill -0 \$(cat tanbo.pid) 2>/dev/null; then
    kill \$(cat tanbo.pid) || true
    sleep 1
  fi
  nohup ./tanbo-server --port 8080 --data $REMOTE_DIR/data --static $REMOTE_DIR/dist > server.out.log 2>&1 &
  echo \$! > tanbo.pid
  echo '已后台启动，日志: '$REMOTE_DIR'/server.out.log'
fi
sleep 1
curl -s -o /dev/null -w '本机自检 HTTP %{http_code}\n' http://127.0.0.1:8080/api/health || true
"

echo ""
echo "完成。浏览器打开：http://$HOST:8080"
echo "记得在云厂商安全组和系统防火墙放行 8080。"

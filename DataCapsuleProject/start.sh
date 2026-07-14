#!/bin/bash
set -e

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
# Must match ports in backend/include/common/serviceAddress.h
CA_PORT=50054
AA_PORT=50057

apt update
apt install -y lsof
apt install -y libmysqlclient-dev
apt install -y libargon2-dev

echo "===== constructing backend... ====="

cd "$ROOT_DIR/backend"

if [ -d cmake/build ]; then
    echo "build has existed!"
else 
    mkdir -p cmake/build
fi

cd cmake/build

if [ ! -f Makefile ]; then
    echo "Generating build configuration..."
    cmake ../..
else
    echo "Build configuration already exists, skip cmake"
fi

make -j4

echo "Starting backend service..."
./ca &
CA_PID=$!
echo "[CA] Waiting for CA service to be available on port ${CA_PORT}..."
while ! lsof -i :${CA_PORT} >/dev/null 2>&1; do
    sleep 1
done
echo "[CA] CA service is up."
./cloud &
./etcd &
./fileServer &
./gateway &
./userServer &
./AA_server &
echo "[AA] Waiting for AA service to be available on port ${AA_PORT}..."
while ! lsof -i :${AA_PORT} >/dev/null 2>&1; do
    sleep 1
done
echo "[AA] AA service is up."
./producer &
./consumer &

sleep 3

echo "===== constructing frontend... ====="

cd "$ROOT_DIR/frontend"
npm install
npm run dev

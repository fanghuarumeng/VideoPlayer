#!/usr/bin/env bash
# Server-only CI gate: configure/build without Qt, run server GTests.
# Usage (from repo root):
#   bash scripts/ci-server.sh
set -euo pipefail
cd "$(dirname "$0")/.."

BUILD_DIR="${BUILD_DIR:-build-server-ci}"
CONFIG="${CONFIG:-Release}"

echo "==> cmake configure (BUILD_CLIENT=OFF)"
cmake -S . -B "$BUILD_DIR" \
  -DBUILD_SERVER=ON \
  -DBUILD_CLIENT=OFF \
  -DCMAKE_BUILD_TYPE="$CONFIG"

echo "==> cmake build"
cmake --build "$BUILD_DIR" --config "$CONFIG" -j"$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 2)"

echo "==> ctest (server)"
ctest --test-dir "$BUILD_DIR" -C "$CONFIG" --output-on-failure

echo "==> server CI green"

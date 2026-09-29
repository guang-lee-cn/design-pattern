#!/bin/bash
# 07 节实验：构建与运行
# 用法：bash build.sh
cd "$(dirname "$0")" || exit 1
BIN=${TMPDIR:-/tmp}/fp07
mkdir -p "$BIN"

echo "=== 编译 ==="
g++ -std=c++17 -O1 -g -fsanitize=address 01_cache_owner.cpp -o "$BIN/01" 2>&1 | head -30
g++ -std=c++23 -O1 02_fail_semantics.cpp -o "$BIN/02" 2>&1 | head -30
echo "--- 编译产物 ---"
ls -1 "$BIN" 2>/dev/null

for m in a a2 b b2 c; do
  echo
  echo "=== 01 模式 $m ==="
  "$BIN/01" "$m" 2>&1
  echo "退出码 = ${PIPESTATUS[0]}"
done

echo
echo "=== 02 四种失败表达 ==="
"$BIN/02" all 2>&1

echo
echo "=== 02 忘记检查 nullptr ==="
"$BIN/02" crash 2>&1
echo "退出码 = $?"

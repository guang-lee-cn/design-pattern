#!/bin/bash
# 03 节「编译耦合」实测：同一份业务逻辑，两种依赖面
# 用法：bash run.sh
cd "$(dirname "$0")" || exit 1

echo "=== 1. 依赖对照：g++ -MM（只列用户头，不含系统头）==="
echo
echo "--- 新世界 business.cpp ---"
g++ -std=c++17 -MM -I. business.cpp
echo
echo "--- 旧世界 business_old.cpp ---"
g++ -std=c++17 -MM -I. business_old.cpp
echo
echo "--- 唯一耦合点 sensor_factory.cpp ---"
g++ -std=c++17 -MM -I. sensor_factory.cpp

echo
echo "=== 2. 全量编译（-MMD 自动生成依赖）==="
make clean >/dev/null 2>&1
make all 2>&1 | sed 's/^/    /'

echo
echo "=== 3. 只改一个具体传感器头：temp_sensor.h ==="
touch temp_sensor.h
echo "    touch 后 make —— 看哪些 TU 被重建："
make all 2>&1 | sed 's/^/    /'

echo
echo "=== 4. 运行 ==="
"${TMPDIR:-/tmp}/fp03_multi_tu"

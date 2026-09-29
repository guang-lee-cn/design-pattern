#!/usr/bin/env bash
# 第 1 节「诞生背景」——全部示例一键编译运行
#
#   bash build.sh
#
# 覆盖：痛点 1/2（old.cpp、spread.cpp）、痛点 3（responsibility.cpp）、
#       痛点 4（testability_before/after.cpp，含 gcov 分支计数）
set -u
CXX=${CXX:-g++}
STD=-std=c++17
HERE=$(cd "$(dirname "$0")" && pwd)
T=/tmp/fp01_build

rm -rf "$T"; mkdir -p "$T"; cd "$T" || exit 1

echo "===== 1) 直接 new：三痛点的最小现场（old.cpp）====="
"$CXX" $STD -O0 -Wall "$HERE/old.cpp" -o old && ./old

echo
echo "===== 2) 修改扩散：4 个 TU 各一份决策（spread.cpp）====="
"$CXX" $STD -O0 -Wall "$HERE/spread.cpp" -o spread && ./spread

echo
echo "===== 3) 责任错位：构造知识泄漏（responsibility.cpp）====="
"$CXX" $STD -O0 -Wall "$HERE/responsibility.cpp" -o resp && ./resp

echo
echo "===== 4) 痛点 4 可测试性：--coverage + gcov ====="
"$CXX" $STD -O0 --coverage "$HERE/testability_before.cpp" -o before || exit 1
"$CXX" $STD -O0 --coverage "$HERE/testability_after.cpp"  -o after  || exit 1

echo "--- 运行 before ---"; ./before
echo "--- 运行 after  ---"; ./after

gcov -b -c before-testability_before.gcno >/dev/null 2>&1
gcov -b -c after-testability_after.gcno  >/dev/null 2>&1

# 只取 over_threshold 里那条 if 的分支，滤掉 printf 调用边噪声
EXTRACT='/^ *[0-9#-]+: *[0-9]+:/{l=$0;p=l;sub(/^ *[0-9#-]+: */,"",p);split(p,q,":");cur=q[1];isif=(l~/if \(/);next}/^ *branch/{if(isif)print "  src_line="cur"  "$0}'
echo
echo "--- before：目标 if 的分支命中（true 侧应为 taken 0）---"
awk "$EXTRACT" testability_before.cpp.gcov
echo "--- after：目标 if 的分支命中（两侧应各 taken 1）---"
awk "$EXTRACT" testability_after.cpp.gcov

cd / && rm -rf "$T"
echo
echo "全部完成。"

#!/usr/bin/env bash
# 第 2 节「不用它的代价」—— 编译运行全部示例，并把「代价」量化出来
#
#   bash build.sh
#
# 覆盖：
#   1) 路线一 硬编码调用     硬编码的两个显示端
#   2) 路线三 持有名单       名单版的两个显示端
#   3) 输出逐行对比         证明重构没有改变行为
#   4) 加第三个显示端        改动面 diff（改了几行 / 改到谁身上）
#   5) 重编面               改公共头 vs 改组合根，各触发几个翻译单元重编
#   6) 路线二 轮询           空转 / 延迟 / 尖峰漏报 / 真实 CPU
set -u

CXX=${CXX:-g++}
STD="-std=c++17 -O2 -Wall"
HERE=$(cd "$(dirname "$0")" && pwd)
T=/tmp/wb02_build
rm -rf "$T"; mkdir -p "$T"; cd "$T" || exit 1

build_run () {   # $1=子目录 $2=输出名
    "$CXX" $STD -I"$HERE/$1" "$HERE/$1/main.cpp" -o "$2" || exit 1
    "./$2"
}

# 两个文件之间新增/删除的行数（含空行，不含 diff 的文件头）
changed_lines () {
    diff -u "$1" "$2" | sed -n '/^@@/,$p' | grep -c '^[+-]'
}

echo "===== 1) 路线一 硬编码调用：两个显示端 ====="
build_run hardcoded hardcoded_v2

echo
echo "===== 2) 路线三 持有名单：两个显示端 ====="
build_run observer observer_v2

echo
echo "--- 两条路线的输出是否逐行相同（证明重构没有改变行为）---"
./hardcoded_v2 > out_v2_h.txt
./observer_v2  > out_v2_o.txt
if diff -u out_v2_h.txt out_v2_o.txt; then
    echo "相同：$(wc -l < out_v2_h.txt) 行全部一致"
fi

echo
echo "===== 3) 加第三个显示端 AlertDisplay ====="
build_run hardcoded_3rd hardcoded_v3
echo
build_run observer_3rd  observer_v3

echo
echo "--- 第三个显示端接入后，两条路线的输出仍逐行相同 ---"
./hardcoded_v3 > out_v3_h.txt
./observer_v3  > out_v3_o.txt
if diff -u out_v3_h.txt out_v3_o.txt; then
    echo "相同：$(wc -l < out_v3_h.txt) 行全部一致"
fi

echo
echo "===== 4) 改动面：加第三个显示端，各文件改了几行 ====="
echo
printf '%-14s %-14s %s\n' "文件" "硬编码调用" "持有名单"
hs=0; os=0
for f in display.h sensor.h main.cpp; do
    h=$(changed_lines "$HERE/hardcoded/$f" "$HERE/hardcoded_3rd/$f")
    o=$(changed_lines "$HERE/observer/$f"  "$HERE/observer_3rd/$f")
    hs=$((hs + h)); os=$((os + o))
    printf '%-14s %-14s %s\n' "$f" "$h" "$o"
done
printf '%-14s %-14s %s\n' "合计" "$hs" "$os"

echo
echo "--- 传感器文件的实际 diff（路线一：必须回来改）---"
( cd "$HERE" && diff -u hardcoded/sensor.h hardcoded_3rd/sensor.h ) || true

echo
echo "--- 传感器文件的实际 diff（路线三：零改动）---"
( cd "$HERE" && diff -u observer/sensor.h observer_3rd/sensor.h ) || true
echo "（上面没有内容 = 两版逐字节相同）"

echo
echo "===== 5) 重编面：同一个改动，落在谁身上 ====="
echo
echo "fanout/ 里 4 个翻译单元（main + a + b + c）都 #include \"sensor.h\"。"
rm -rf "$T/fanout"; cp -r "$HERE/fanout" "$T/fanout"
cd "$T/fanout" || exit 1
make -s >/dev/null 2>&1 || exit 1

report () {
    local out; out=$(make 2>&1)
    printf '%s\n' "$out" | sed 's/^/    /'
    echo "    → 重编 TU 数：$(printf '%s\n' "$out" | grep -c 'CXX ')"
}

echo
echo "--- 改 sensor.h（被 4 个 TU 直接依赖的公共头）---"
touch sensor.h; report
echo
echo "--- 改 main.cpp（只被 1 个 TU 依赖的组合根）---"
touch main.cpp; report
echo
echo "--- 改 display.h（4 个 TU 经 sensor.h 间接依赖）---"
touch display.h; report

cd "$T" || exit 1

echo
echo "===== 6) 路线二 轮询：空转 / 延迟 / 尖峰漏报 / 真实 CPU ====="
echo
"$CXX" $STD "$HERE/polling.cpp" -o polling || exit 1
./polling

cd / || exit 1
rm -rf "$T"
echo
echo "全部完成。"

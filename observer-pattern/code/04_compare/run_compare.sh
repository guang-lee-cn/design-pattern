#!/bin/sh
# 第 4 节 · 同一份传感器场景，四层实现并排跑。
#
# 四个源文件由使用者录入，本脚本只负责三件事：
#   1. 逐层编译并运行（四层跑的是同一条事件序列，见下面的 banner）
#   2. 给出「分发时机」的逐行证据 —— Publish 返回时，接收方到底执行完没有
#   3. 给出「可数性」的机器证据 —— 发布方这一侧有没有「数出订阅者人数」的代码
#
# 用法（在本目录下）：sh run_compare.sh
set -u

CXX=${CXX:-g++}
STD=-std=c++17
WARN="-Wall -Wextra"
HERE=$(cd "$(dirname "$0")" && pwd)
OUT="${TMPDIR:-/tmp}/observer_04_compare"
mkdir -p "$OUT"

# 逐层编译 + 运行。$2 是额外的链接参数（只有第 4 层需要 -pthread）。
run_layer() {
  name=$1
  link=$2
  src="$HERE/$name.cpp"
  printf '\n===== %s =====\n' "$name"
  if [ ! -f "$src" ]; then
    printf '  [SKIP] %s 尚未录入\n' "$name.cpp"
    return
  fi
  # shellcheck disable=SC2086
  if ! $CXX $STD $WARN $link "$src" -o "$OUT/$name" 2>"$OUT/$name.err"; then
    printf '  [FAIL] 编译失败：\n'
    sed 's/^/    /' "$OUT/$name.err" | head -20
    return
  fi
  if [ -s "$OUT/$name.err" ]; then
    printf '  [WARN] -Wall -Wextra 有输出：\n'
    sed 's/^/    /' "$OUT/$name.err" | head -20
  fi
  "$OUT/$name"
}

printf '### 四层并排跑\n'
printf '### 场景：两个显示端都在听，传感器报 25.0 / 26.5 / 27.0；\n'
printf '###       然后撤销 audit 的订阅，再报 28.0（只应有 console 收到）。\n'

run_layer layer1_callback ''
run_layer layer2_observer ''
run_layer layer3_eventbus ''
run_layer layer4_pubsub '-pthread'

printf '\n===== 可数性证据：发布方这一侧有没有「数出人数」的代码 =====\n'
printf '  %-22s %s\n' '文件' '匹配行数'
for n in layer1_callback layer2_observer layer3_eventbus layer4_pubsub; do
  src="$HERE/$n.cpp"
  if [ -f "$src" ]; then
    c=$(grep -cE 'observer_count|receiver_count' "$src")
  else
    c='-'
  fi
  printf '  %-22s %s\n' "$n.cpp" "$c"
done
printf '\n（判据：只有「发布方自己持有名单」的那一层才数得出人数。）\n'

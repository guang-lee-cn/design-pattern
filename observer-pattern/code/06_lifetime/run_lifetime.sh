#!/bin/sh
# 生命周期负债的三个实测（观察者模式第 6 节）。
#
# 三个例子各自独立，每个都跑两遍 —— 不带 sanitizer 一遍，带 -fsanitize=address 一遍。
# 「报不报得出来」本身就是这三条证据的一部分，所以脚本不只跑，还要判定：
# 两遍的 stdout 是否逐字节相同（cmp）。
#
#   1 dangling.cpp                悬垂指针。变体 A 地址被复用 ⇒ 通知发给没订阅的对象，
#                                 任何工具都不报；变体 B 地址留空 ⇒ ASan 报 use-after-free。
#   2 mutation_during_notify.cpp  通知期间改名单。漏发 + 重复，两遍输出逐字相同 ——
#                                 内存工具抓不到协议错误。
#   3 reference_cycle.cpp         引用环。析构函数一次都不跑；ASan 的泄漏检测报出来。
#
# 用法: sh run_lifetime.sh
# 环境变量: CXX 指定编译器（默认 g++）
set -e

CXX="${CXX:-g++}"
OUT="$(mktemp -d)"
trap 'rm -rf "$OUT"' EXIT

# 编译并运行一份源码。参数: $1=源文件 $2=输出标签 $3=sanitizer 参数（可为空）
one_round() {
  file="$1"
  tag="$2"
  sanitizer="$3"

  # shellcheck disable=SC2086
  "$CXX" -std=c++17 -Wall -Wextra -Werror $sanitizer -g "$file" -o "$OUT/$tag"

  set +e
  "$OUT/$tag" >"$OUT/$tag.out" 2>&1
  printf '%s' "$?" >"$OUT/$tag.code"
  set -e
}

table="$OUT/summary"
: >"$table"
for src in dangling.cpp mutation_during_notify.cpp reference_cycle.cpp; do
  base="${src%.cpp}"
  printf '\n##################### %s #####################\n' "$src"

  one_round "$src" "$base.plain" ""
  printf '\n----- no sanitizer (exit %s) -----\n' "$(cat "$OUT/$base.plain.code")"
  head -n 70 "$OUT/$base.plain.out"

  one_round "$src" "$base.asan" "-fsanitize=address"
  printf '\n----- -fsanitize=address (exit %s) -----\n' "$(cat "$OUT/$base.asan.code")"
  head -n 26 "$OUT/$base.asan.out"

  if cmp -s "$OUT/$base.plain.out" "$OUT/$base.asan.out"; then
    verdict="identical"
  else
    verdict="differs"
  fi
  printf '\n>>> stdout: %s | exit code: %s vs %s\n' "$verdict" \
    "$(cat "$OUT/$base.plain.code")" "$(cat "$OUT/$base.asan.code")"
  printf '%-30s %-8s %-8s %s\n' "$src" \
    "$(cat "$OUT/$base.plain.code")" "$(cat "$OUT/$base.asan.code")" "$verdict" >>"$table"
done

printf '\n=========== summary ===========\n'
printf '%-30s %-8s %-8s %s\n' "file" "plain" "asan" "stdout"
cat "$table"

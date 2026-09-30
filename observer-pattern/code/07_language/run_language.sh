#!/bin/sh
# 第 7 节：三代接口形状并排 —— 差异只允许出现在编译期。
#
# 六件事，按序做：
#   1. 三代各自构建并运行，用 cmp 证明运行输出逐字节相同
#   2. 编译期拦截矩阵：7 个反例 × 3 代（结果 + 首条错误落点 + error 行数）
#   3. 身份：第 1 代的指针可比较，第 2/3 代的 std::function 不可比较
#   4. 生产者代码里还叫得出接收方的名字吗（接收方名在 Sensor 类体内的出现次数）
#   5. 类型擦除代价：operator new 计数扫出 std::function 的内联阈值
#   6. 同一个约束的两种写法（SFINAE / concepts）的诊断对照
#
# 用法：在本目录下执行 sh run_language.sh
set -u

CXX=${CXX:-g++}
OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT

# 第 3 代要 C++20 的 requires 子句；第 1、2 代用 C++17
std_for_gen() {
  if [ "$1" = "3" ]; then
    echo "-std=c++20"
  else
    echo "-std=c++17"
  fi
}

# 取首条 error 的位置。
#
# g++ 的格式是 file:line:col: error: —— 注意最后一段是**列号**不是行号，
# 按 "最后一段" 取会得到列号（踩过：identity.cpp 的行 20 被读成 24）。
# 所以先剥掉 ": error:"，再取「倒数第二段」当行号。
first_error_loc() {
  awk '/: error:/ { sub(/: error:.*/, "", $0); print; exit }' "$1"
}

# 错误落在哪一类位置：约束声明 / 调用方 / 标准库
classify_error() {
  case "$1" in
    examples/interfaces.hpp) echo "api" ;;
    /usr/*|*/c++/*) echo "lib" ;;
    *) echo "user" ;;
  esac
}

describe_error() {
  loc=$(first_error_loc "$1")
  if [ -z "$loc" ]; then
    echo "-"
    return
  fi
  file=${loc%:*:*}
  line=$(printf '%s' "${loc%:*}" | awk -F: '{ print $NF }')
  printf '%s:%s' "$(classify_error "$file")" "$line"
}

# 首条 error 的原文（去掉前缀，只留消息）
first_error_text() {
  awk '/: error:/ { sub(/^.*: error: /, ""); print; exit }' "$1"
}

# error 行数 —— 诊断噪声的代理指标
count_errors() {
  awk '/: error:/ { n++ } END { print n + 0 }' "$1"
}

printf '################################################################\n'
printf '# 1. 三代各自构建并运行（同一份场景，输出必须逐字节相同）\n'
printf '################################################################\n'
for gen in 1 2 3; do
  src="gen${gen}_"
  case "$gen" in
    1) src="gen1_virtual.cpp" ;;
    2) src="gen2_function.cpp" ;;
    3) src="gen3_concept.cpp" ;;
  esac
  std=$(std_for_gen "$gen")
  if $CXX $std -Wall -Wextra "$src" -o "$OUT/g$gen" 2>"$OUT/build$gen.log"; then
    "$OUT/g$gen" >"$OUT/run$gen.out" 2>&1
    printf '\n----- GEN%s (%s) exit 0 / 输出 %s 行 -----\n' \
      "$gen" "$std" "$(wc -l <"$OUT/run$gen.out" | tr -d ' ')"
    cat "$OUT/run$gen.out"
  else
    printf '\n----- GEN%s (%s) 构建失败 -----\n' "$gen" "$std"
    cat "$OUT/build$gen.log"
  fi
done

printf '\n>>> 运行输出比对\n'
if cmp -s "$OUT/run1.out" "$OUT/run2.out"; then
  printf '    GEN1 vs GEN2 : 逐字节相同\n'
else
  printf '    GEN1 vs GEN2 : 不同\n'
fi
if cmp -s "$OUT/run1.out" "$OUT/run3.out"; then
  printf '    GEN1 vs GEN3 : 逐字节相同\n'
else
  printf '    GEN1 vs GEN3 : 不同\n'
fi

printf '\n################################################################\n'
printf '# 2. 编译期拦截矩阵：7 个反例 × 3 代\n'
printf '################################################################\n'
printf '%-32s %-14s %-14s %-14s\n' "example" "GEN1 C++17" "GEN2 C++17" "GEN3 C++20"
for src in examples/[0-9][0-9]_*.cpp; do
  name=$(basename "$src")
  printf '%-32s' "$name"
  for gen in 1 2 3; do
    log="$OUT/$(basename "$src" .cpp).g$gen.log"
    std=$(std_for_gen "$gen")
    if $CXX $std -Wall -Wextra -DGEN=$gen -Iexamples "$src" -o "$OUT/ex.out" 2>"$log"; then
      cell="PASS"
    else
      cell="FAIL $(describe_error "$log")"
    fi
    printf '%-14s' "$cell"
  done
  printf '\n'
done

printf '\n>>> 诊断噪声（error 行数）\n'
printf '%-32s %-14s %-14s %-14s\n' "example" "GEN1" "GEN2" "GEN3"
for src in examples/[0-9][0-9]_*.cpp; do
  name=$(basename "$src")
  printf '%-32s' "$name"
  for gen in 1 2 3; do
    log="$OUT/$(basename "$src" .cpp).g$gen.log"
    printf '%-14s' "$(count_errors "$log")"
  done
  printf '\n'
done

printf '\n>>> 每一格 FAIL 的首条错误原文（矩阵的可核部分）\n'
for src in examples/[0-9][0-9]_*.cpp; do
  name=$(basename "$src")
  for gen in 1 2 3; do
    log="$OUT/$(basename "$src" .cpp).g$gen.log"
    if [ "$(count_errors "$log")" != "0" ]; then
      printf '  %-30s GEN%s  %-9s  %s\n' "$name" "$gen" "$(describe_error "$log")" \
        "$(first_error_text "$log")"
    fi
  done
done

printf '\n################################################################\n'
printf '# 3. 身份：生产者手里那件东西能不能指认某一个接收方\n'
printf '################################################################\n'
for gen in 1 2 3; do
  std=$(std_for_gen "$gen")
  log="$OUT/identity$gen.log"
  if $CXX $std -DGEN=$gen identity.cpp -o "$OUT/id" 2>"$log"; then
    printf 'GEN%s  PASS  %s\n' "$gen" "可以比较"
  else
    printf 'GEN%s  FAIL  %s\n' "$gen" "$(describe_error "$log")"
    printf '      首条错误：%s\n' "$(first_error_text "$log")"
  fi
done

printf '\n################################################################\n'
printf '# 4. 接收方的名字还出现在生产者类体里吗\n'
printf '################################################################\n'
printf '%s\n' 'gen1_virtual.cpp 的 Sensor 类体内：'
awk '/^class Sensor \{/,/^\};/' gen1_virtual.cpp | grep -c 'Observer' | sed 's/^/    Observer 出现 /; s/$/ 次/'
printf '%s\n' 'gen2_function.cpp 的 Sensor 类体内：'
awk '/^class Sensor \{/,/^\};/' gen2_function.cpp | grep -c 'ConsoleDisplay\|AuditDisplay' | sed 's/^/    接收方类型名出现 /; s/$/ 次/'
printf '%s\n' '（全文中接收方类型名出现的位置，gen2：）'
grep -n 'ConsoleDisplay\|AuditDisplay' gen2_function.cpp | sed 's/^/    /'

printf '\n################################################################\n'
printf '# 5. 类型擦除代价：std::function 的内联阈值\n'
printf '################################################################\n'
if $CXX -std=c++17 -Wall -Wextra allocation_cost.cpp -o "$OUT/alloc" 2>"$OUT/alloc.log"; then
  "$OUT/alloc"
else
  cat "$OUT/alloc.log"
fi

printf '\n################################################################\n'
printf '# 6. 悬垂：接口换过之后，同一个负债\n'
printf '################################################################\n'
for mode in plain asan; do
  if [ "$mode" = "asan" ]; then
    flags="-fsanitize=address -g"
  else
    flags=""
  fi
  # shellcheck disable=SC2086
  if $CXX -std=c++17 -Wall -Wextra $flags dangling_sink.cpp -o "$OUT/d7_$mode" 2>"$OUT/d7_$mode.build"; then
    "$OUT/d7_$mode" >"$OUT/d7_$mode.out" 2>&1
    code=$?
    printf '\n----- %s (exit %s) -----\n' "$mode" "$code"
    sed -n '1,12p' "$OUT/d7_$mode.out"
  else
    printf '\n----- %s 构建失败 -----\n' "$mode"
    cat "$OUT/d7_$mode.build"
  fi
done
if cmp -s "$OUT/d7_plain.out" "$OUT/d7_asan.out"; then
  printf '\n>>> stdout: identical\n'
else
  printf '\n>>> stdout: differs\n'
fi

printf '\n################################################################\n'
printf '# 7. 同一个约束，两种写法：SFINAE 与 concepts 的诊断对照\n'
printf '################################################################\n'
for sp in 1 2; do
  if [ "$sp" = "2" ]; then
    std="-std=c++20"
    label="concepts (C++20)"
  else
    std="-std=c++17"
    label="SFINAE   (C++17)"
  fi
  log="$OUT/sp$sp.log"
  if $CXX $std -Wall -Wextra -DSPELLING=$sp sfinae_vs_concept.cpp -o "$OUT/sp" 2>"$log"; then
    printf '%s  意外编译通过\n' "$label"
  else
    printf '\n----- SPELLING=%s  %s -----\n' "$sp" "$label"
    printf '  error 行数 : %s\n' "$(count_errors "$log")"
    printf '  总输出行数 : %s\n' "$(wc -l <"$log" | tr -d ' ')"
    printf '  首条位置   : %s\n' "$(describe_error "$log")"
    printf '  首条原文   : %s\n' "$(first_error_text "$log")"
  fi
done
printf '\n>>> 约束相关的 note 行数\n'
for sp in 1 2; do
  printf '  SPELLING=%s : %s\n' "$sp" "$(awk '/: note:/ { n++ } END { print n + 0 }' "$OUT/sp$sp.log")"
done

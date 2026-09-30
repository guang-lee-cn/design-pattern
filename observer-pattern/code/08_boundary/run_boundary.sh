#!/bin/sh
# 第 8 节：六种形态的边界取证
#
# 一次跑完四件事：
#   1. 编译并运行六个形态，各自的完整轨迹存进 out/
#   2. 以 form A 为基准做逐字节对照（cmp），给出首条差异落在第几轮
#   3. 结构取证：五条判据各自落在哪个文件的哪一行
#   4. 每轮实际触达几个接收方（可数证据）
#
# 脚本只取证，不下结论 —— 结论由正文根据这里打印的原始材料写。
# 可重复运行：产物落在 mktemp 出来的临时目录里，不污染仓库。

set -e
cd "$(dirname "$0")"

OUT=$(mktemp -d)
trap 'rm -rf "$OUT"' EXIT

CXX=${CXX:-g++}
FLAGS="-std=c++17 -Wall -Wextra"

FORMS="form_a_observer_push form_b_observer_pull form_c_change_manager form_d_one_shot form_e_chain form_f_pure_pull"
BASE=form_a_observer_push

printf '##################### 1. compile and run #####################\n'
for f in $FORMS; do
  printf '\n===== %s =====\n' "$f.cpp"
  $CXX $FLAGS "$f.cpp" -o "$OUT/$f"
  "$OUT/$f" > "$OUT/$f.txt"
  cat "$OUT/$f.txt"
done

printf '\n\n##################### 2. compare against form A #####################\n'
printf '%-26s %-10s %-12s %s\n' form cmp at '(in round)'
for f in $FORMS; do
  [ "$f" = "$BASE" ] && continue
  if cmp -s "$OUT/$BASE.txt" "$OUT/$f.txt"; then
    printf '%-26s %-10s %-12s %s\n' "$f" identical - -
    continue
  fi
  at=$(diff "$OUT/$BASE.txt" "$OUT/$f.txt" | awk '/^[0-9]/{print; exit}')
  line=$(printf '%s' "$at" | sed 's/[^0-9].*//')
  # 首条差异行号取自 form A；该行落在第几轮 = 到这一行为止出现过几个 tick 头
  round=$(awk -v n="$line" \
    'NR <= n && /^-- tick [0-9]/ && !/returned$/ {c++} END {print (c > 0 ? c : 1)}' \
    "$OUT/$BASE.txt")
  printf '%-26s %-10s %-12s %s\n' "$f" differs "$at" "$round"
done

printf '\n--- first differing block, verbatim ---\n'
for f in $FORMS; do
  [ "$f" = "$BASE" ] && continue
  printf '\n===== %s vs form A =====\n' "$f"
  if cmp -s "$OUT/$BASE.txt" "$OUT/$f.txt"; then
    printf '(identical -- no line differs)\n'
  else
    diff "$OUT/$BASE.txt" "$OUT/$f.txt" | head -n 12
  fi
done

printf '\n\n##################### 3. structural evidence #####################\n'

printf '\n--- Q1  where does the receiver list live? ---\n'
grep -n 'std::vector<Observer\*> observers_' *.cpp
grep -n 'std::vector<Subscription> subscriptions_' *.cpp
grep -n 'std::vector<Handler> pending_' *.cpp
printf '  [form F matches none of the above -- it has no list]\n'

printf '\n--- Q2  what travels with the call? ---\n'
grep -n 'virtual void OnReading\|virtual bool OnReading\|using Handler = std::function' *.cpp
grep -n 'void Set(double value)' *.cpp

printf '\n--- Q3  is the answer to a notification read? ---\n'
grep -n 'virtual bool OnReading' *.cpp
grep -n 'if (observer->OnReading' *.cpp

printf '\n--- Q4  is registration permanent until revoked? ---\n'
grep -n 'void Detach\|void Unregister\|batch.swap(pending_)' *.cpp

printf '\n--- Q5  does the producer call anybody at all? ---\n'
grep -n 'observer->OnReading\|handler(value)\|\.Poll(sensor)' *.cpp


printf '\n\n##################### 4. receivers reached per round #####################\n'
printf '%-26s %s\n' form 'r1  r2  r3  r4'
for f in $FORMS; do
  printf '%-26s' "$f"
  awk '/^-- (tick|cycle) [0-9]/ && !/done$|returned$/ {r++} /^\[/ {c[r]++} \
       END {for (i = 1; i <= 4; i++) printf "  %d ", c[i] + 0; print ""}' "$OUT/$f.txt"
done

printf '\n\n##################### 5. why a mediator: one observer, two subjects #####################\n'
$CXX $FLAGS mediator_dedup.cpp -o "$OUT/mediator_dedup"
"$OUT/mediator_dedup" | tee "$OUT/mediator_dedup.txt"

printf '\n\n##################### 6. the entries change shape: names vs rules #####################\n'
$CXX $FLAGS topic_wildcard.cpp -o "$OUT/topic_wildcard"
"$OUT/topic_wildcard" | tee "$OUT/topic_wildcard.txt"

#!/usr/bin/env bash
# A04 · C 版工厂实验：一条命令跑全部
# 产物放 /tmp/factory-pattern-c（临时草稿区，下电即清；不污染仓库）
set -e

SRC="$(cd "$(dirname "$0")" && pwd)"
OUT="/tmp/factory-pattern-c"
mkdir -p "$OUT"

CFLAGS="-std=c11 -Wall -Wextra -O1"

echo "########## 01 手工 vtable + 两档工厂 ##########"
gcc $CFLAGS "$SRC/01_vtable_and_factory.c" -o "$OUT/01"
"$OUT/01"

echo
echo "########## 02 族表（C 版抽象工厂）##########"
gcc $CFLAGS "$SRC/02_family_table.c" -o "$OUT/02"
"$OUT/02"

echo
echo "########## 03/04 链接器段自注册 ##########"
gcc $CFLAGS -c "$SRC/03_self_register_main.c" -o "$OUT/main.o"
gcc $CFLAGS -c "$SRC/04_self_register_gyro.c" -o "$OUT/gyro.o"
ar rcs "$OUT/libsensors.a" "$OUT/gyro.o"
echo "静态库成员：$(ar t "$OUT/libsensors.a" | tr '\n' ' ')"

echo
echo "--- 链接方式 A：普通静态库链接 ---"
gcc $CFLAGS "$OUT/main.o" "$OUT/libsensors.a" -o "$OUT/03a"
"$OUT/03a"

echo
echo "--- 链接方式 B：--whole-archive（强制拉入 .a 全部成员）---"
gcc $CFLAGS "$OUT/main.o" -Wl,--whole-archive "$OUT/libsensors.a" -Wl,--no-whole-archive -o "$OUT/03b"
"$OUT/03b"

echo
echo "########## 全部实验完成 ##########"

#!/bin/sh
# build.sh —— 跑 A05 全部实验（产物落 build/，该目录已在 .gitignore 中）
# 用法：sh build.sh

set -e

DIR=$(cd "$(dirname "$0")" && pwd)
OUT="$DIR/build"
mkdir -p "$OUT"

echo "############ 01  offset + 通用 setter（nginx 配置指令表） ############"
g++ -std=c++17 -Wall "$DIR/01_offset_setter.c" -o "$OUT/01"
"$OUT/01"

echo
echo "############ 02  两阶段分派：名字->索引（nginx 事件后端） ############"
g++ -std=c++17 -O2 -Wall "$DIR/02_two_phase_dispatch.c" -o "$OUT/02"
"$OUT/02"

echo
echo "############ 03  表即协议：一表六用（redis 命令表） ############"
g++ -std=c++17 -Wall "$DIR/03_meta_table.c" -o "$OUT/03"
"$OUT/03"

echo
echo "############ 04  构建期代码生成（nginx auto/modules） ############"
echo "（这一组必须用 gcc 而非 g++：C 里 const 全局变量是外部链接，"
echo "  C++ 里默认是内部链接，用 g++ 编纯 C 代码会 undefined reference）"
sh "$DIR/04_gen_modules.sh" "$OUT"
gcc -std=c11 -Wall "$DIR/04_main.c" "$DIR/mod_core.c" "$DIR/mod_http.c" \
    "$DIR/mod_log.c" "$OUT/generated_modules.c" -I"$DIR" -o "$OUT/04a"
"$OUT/04a"

echo
echo "--- 现在只改清单一的 1 行：追加 mod_cache（新模块是一个自包含 .c）---"
cp "$DIR/04_modules.list" "$OUT/list4"
echo "mod_cache" >> "$OUT/list4"

cat > "$OUT/mod_cache.c" <<'EOF'
#include "modules.h"
static int cache_init(void)      { return 400; }
static int cache_handle(int req) { return req + 4; }
const module_t mod_cache = { "cache", cache_init, cache_handle };
EOF

sh "$DIR/04_gen_modules.sh" "$OUT" "$OUT/list4"
gcc -std=c11 -Wall "$DIR/04_main.c" "$DIR/mod_core.c" "$DIR/mod_http.c" \
    "$DIR/mod_log.c" "$OUT/mod_cache.c" "$OUT/generated_modules.c" -I"$DIR" -o "$OUT/04b"
"$OUT/04b"
echo "（04_main.c 这次编译用的是完全相同的文件，一个字节都没改）"

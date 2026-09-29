#!/bin/sh
# 04_gen_modules.sh —— 构建期代码生成（nginx auto/modules 的做法）
#
# 出处：nginx 1.31.7  auto/modules:1594-1615
#     for mod in $modules
#     do
#         echo "extern ngx_module_t  $mod;"   >> $NGX_MODULES_C
#     done
#     echo 'ngx_module_t *ngx_modules[] = {'  >> $NGX_MODULES_C
#     for mod in $modules
#     do
#         echo "    &$mod,"                   >> $NGX_MODULES_C
#     done
#     echo '    NULL'                         >> $NGX_MODULES_C
#     echo '};'                               >> $NGX_MODULES_C
#
# 要点：模块清单（$modules）由配置脚本决定，C 数组是**生成物**。
#       模块作者只写自己的模块文件，不需要碰任何中心清单。
#
# 本脚本等比缩小地复现这套机制。
# 用法：sh 04_gen_modules.sh <输出目录>

set -e

SRC_DIR=$(cd "$(dirname "$0")" && pwd)
OUT_DIR=${1:-$SRC_DIR/build}
LIST=${2:-$SRC_DIR/04_modules.list}
GEN="$OUT_DIR/generated_modules.c"

mkdir -p "$OUT_DIR"

# 读清单：每行一个模块名，忽略注释与空行
modules=""
while read -r line; do
    case "$line" in
        ''|\#*) continue ;;
    esac
    modules="$modules $line"
done < "$LIST"

# ---- 生成 C 文件（这一段与 auto/modules 同构）----
{
    echo '/* 由 04_gen_modules.sh 生成，请勿手改——对应 nginx 的 ngx_modules.c */'
    echo '#include "modules.h"'
    echo
    for m in $modules; do
        echo "extern const module_t $m;"
    done
    echo
    echo 'const module_t *g_modules[] = {'
    for m in $modules; do
        echo "    &$m,"
    done
    echo '    NULL'
    echo '};'
    echo
    echo 'const char *g_module_names[] = {'
    for m in $modules; do
        echo "    \"$m\","
    done
    echo '    NULL'
    echo '};'
} > "$GEN"

n=$(echo $modules | wc -w)
echo "已生成 $GEN（$n 个模块）"
echo "模块清单：$modules"

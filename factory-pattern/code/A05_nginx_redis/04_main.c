/*
 * 04_main.c —— 消费生成的模块数组
 *
 * 注意本文件里**没有出现任何具体模块的名字**：
 * 没有 mod_core、没有 mod_http、没有 mod_log。
 * 它只认识 g_modules[] / g_module_names[] 这两个生成出来的符号。
 *
 * 这就回答了「新增一个模块要改几处」：
 *   手写数组的做法   改数组 + 加 extern 声明 + 改名字数组 = 3 处
 *   构建期生成       改清单一行，重新生成即可       = 1 处，且不碰 main.c
 */
#include "modules.h"
#include <stdio.h>

int main(void) {
    int n = 0;

    printf("=== 由生成数组驱动的模块表 ===\n");
    for (int i = 0; g_modules[i]; ++i) {
        ++n;
        printf("  [%d] %-8s init() = %3d   handle(42) = %d\n",
               i, g_module_names[i],
               g_modules[i]->init(), g_modules[i]->handle(42));
    }
    printf("共 %d 个模块。main.c 里没有任何具体模块名。\n", n);
    return 0;
}

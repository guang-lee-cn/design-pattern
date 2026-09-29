/*
 * modules.h —— 模块对外契约（对应 nginx 的 ngx_module_t 的极简版）
 *
 * nginx 的 ngx_module_t 有 10 个字段（ctx / commands / type / ctx_index / index
 * / spare0..3 / name），这里只保留能说明机制的 3 个。
 */
#ifndef MODULES_H
#define MODULES_H

#include <stddef.h>   /* NULL —— 生成的数组里要用 */

typedef struct {
    const char *name;
    int (*init)(void);
    int (*handle)(int req);
} module_t;

/* 由 04_gen_modules.sh 生成，不在任何源文件里手写 */
extern const module_t *g_modules[];
extern const char   *g_module_names[];

#endif /* MODULES_H */

/*
 * 01_offset_setter.c —— nginx 配置指令表的技巧：表里存「字段偏移」而非「处理函数」
 *
 * 出处：nginx 1.31.7  src/core/ngx_conf_file.h:77-84   结构定义
 *                      src/core/ngx_conf_file.c:1173     ngx_conf_set_num_slot
 *                      src/http/ngx_http_core_module.c:183 使用示例
 *
 * 核心那一行（ngx_conf_file.c:1182）：
 *     np = (ngx_int_t *) (p + cmd->offset);
 * 函数体完全不认识任何具体字段名，只认 offset。
 *
 * 本程序把两种写法放在一起，量化差异。
 * 编译：g++ -std=c++17 -Wall 01_offset_setter.c -o /tmp/a05/01
 */

#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>

/* ============ 被配置的目标结构 ============ */
typedef struct {
    int worker_processes;
    int keepalive_timeout;
    int client_body_size;
    int types_hash_max_size;
    int server_names_hash_max_size;
    int open_file_cache_errors;
} conf_t;

/* =========================================================
 * 方案 A：逐项 setter（"我肯定要认识每个字段"的写法）
 *         6 个配置项 = 6 个函数；加第 7 项就要写第 7 个函数
 * ========================================================= */
static void setA_worker_processes(conf_t *c, const char *v) { c->worker_processes = atoi(v); }
static void setA_keepalive_timeout(conf_t *c, const char *v) { c->keepalive_timeout = atoi(v); }
static void setA_client_body_size(conf_t *c, const char *v) { c->client_body_size = atoi(v); }
static void setA_types_hash_max_size(conf_t *c, const char *v) { c->types_hash_max_size = atoi(v); }
static void setA_server_names_hash_max_size(conf_t *c, const char *v) { c->server_names_hash_max_size = atoi(v); }
static void setA_open_file_cache_errors(conf_t *c, const char *v) { c->open_file_cache_errors = (strcmp(v, "on") == 0); }

/* =========================================================
 * 方案 B：nginx 的做法 —— 2 个通用 setter + offset 数据
 *         函数体只做「指针算术定位」，不含任何字段名
 * ========================================================= */
typedef struct {
    const char *name;
    void      (*set)(void *conf, size_t offset, const char *value);
    size_t      offset;
} cmd_t;

/* 通用 setter 之一：数值。等价于 ngx_conf_set_num_slot */
static void set_int(void *conf, size_t offset, const char *value) {
    int *np = (int *)((char *)conf + offset);   /* ★ 与 nginx 同一行语义 */
    if (*np != -1) {                            /* 对应 nginx 的 "is duplicate" 检查 */
        fprintf(stderr, "[warn] duplicate directive\n");
    }
    *np = atoi(value);
}

/* 通用 setter 之二：开关。等价于 ngx_conf_set_flag_slot */
static void set_flag(void *conf, size_t offset, const char *value) {
    int *np = (int *)((char *)conf + offset);
    *np = (strcmp(value, "on") == 0);
}

static const cmd_t cmds[] = {
    { "worker_processes",           set_int,  offsetof(conf_t, worker_processes) },
    { "keepalive_timeout",          set_int,  offsetof(conf_t, keepalive_timeout) },
    { "client_body_size",           set_int,  offsetof(conf_t, client_body_size) },
    { "types_hash_max_size",        set_int,  offsetof(conf_t, types_hash_max_size) },
    { "server_names_hash_max_size", set_int,  offsetof(conf_t, server_names_hash_max_size) },
    { "open_file_cache_errors",     set_flag, offsetof(conf_t, open_file_cache_errors) },
    { NULL, NULL, 0 }
};

/* 方案 A 的现实形态：名字 → 专用函数 的一张表（写成 switch 完全等价） */
typedef struct {
    const char *name;
    void      (*set)(conf_t *, const char *);
} cmdA_t;

static const cmdA_t cmdsA[] = {
    { "worker_processes",           setA_worker_processes },
    { "keepalive_timeout",          setA_keepalive_timeout },
    { "client_body_size",           setA_client_body_size },
    { "types_hash_max_size",        setA_types_hash_max_size },
    { "server_names_hash_max_size", setA_server_names_hash_max_size },
    { "open_file_cache_errors",     setA_open_file_cache_errors },
    { NULL, NULL }
};

int main(void) {
    printf("=== A. 逐项 setter vs B. 通用 setter + offset ===\n\n");

    /* ---- 方案 A 跑一遍：6 个字段 = 6 个专用函数 ---- */
    conf_t ca;
    memset(&ca, 0xff, sizeof(ca));

    const char *rtA[] = { "worker_processes", "4",
                          "keepalive_timeout", "65",
                          "client_body_size", "1048576",
                          "open_file_cache_errors", "on" };

    for (size_t i = 0; i < sizeof(rtA) / sizeof(rtA[0]); i += 2) {
        for (const cmdA_t *p = cmdsA; p->name; ++p) {
            if (strcmp(p->name, rtA[i]) == 0) { p->set(&ca, rtA[i + 1]); break; }
        }
    }
    printf("方案 A 结果（每个字段一个专用函数，函数体写死了字段名）:\n");
    printf("  worker_processes=%d  keepalive_timeout=%d  client_body_size=%d  open_file_cache_errors=%d\n\n",
           ca.worker_processes, ca.keepalive_timeout, ca.client_body_size, ca.open_file_cache_errors);

    /* ---- 方案 B 实际跑一遍 ---- */
    conf_t c;
    memset(&c, 0xff, sizeof(c));          /* 全填 -1 = NGX_CONF_UNSET */

    const char *rt[] = { "worker_processes", "4",
                         "keepalive_timeout", "65",
                         "client_body_size", "1048576",
                         "open_file_cache_errors", "on" };

    for (size_t i = 0; i < sizeof(rt) / sizeof(rt[0]); i += 2) {
        for (const cmd_t *p = cmds; p->name; ++p) {
            if (strcmp(p->name, rt[i]) == 0) {
                p->set(&c, p->offset, rt[i + 1]);
                break;
            }
        }
    }

    printf("方案 B 写入结果（全部经「偏移量」定位，setter 不知道字段名）:\n");
    printf("  worker_processes           = %d\n", c.worker_processes);
    printf("  keepalive_timeout          = %d\n", c.keepalive_timeout);
    printf("  client_body_size           = %d\n", c.client_body_size);
    printf("  types_hash_max_size        = %d  (未配置，仍是 UNSET)\n", c.types_hash_max_size);
    printf("  server_names_hash_max_size = %d  (未配置，仍是 UNSET)\n", c.server_names_hash_max_size);
    printf("  open_file_cache_errors     = %d\n", c.open_file_cache_errors);

    printf("\n=== 扩展成本：新增第 7 项配置 ===\n");
    printf("方案 A：写一个新 setter 函数 + 表里加一行 + 改 switch  → 3 处\n");
    printf("方案 B：表里加一行（复用 set_int）                      → 1 处\n");

    printf("\n=== 同一技巧在真实 nginx 里的规模 ===\n");
    printf("通用 setter 家族：12 个（flag/str/str_array/keyval/num/size/off/msec/sec/bufs/enum/bitmask）\n");
    printf("command 表：107 张（全仓），仅 http_core 模块一张表就有 124 个配置项\n");
    printf("=> 124 项配置由 12 个函数服务；若逐项写 setter，就是 124 个函数\n");
    return 0;
}

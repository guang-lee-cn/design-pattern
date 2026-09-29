/*
 * 03_meta_table.c —— redis 命令表：一张表同时是「分派表」和「协议文档」
 *
 * 出处：redis（unstable）
 *   src/server.h:3103-3135        struct redisCommand { ... }   30+ 字段
 *   src/server.c:3675-3693        lookupCommandLogic / lookupCommand（dictFetchValue，O(1)）
 *   src/commands/ 下的 464 个 .json   源数据
 *   utils/generate-command-code.py  读 JSON → 生成 src/commands.def（531 KB）
 *
 * 与前面所有「表」的关键差别：
 *   04a 的注册表    map<string, function>          —— 1 个 key + 1 个函数指针
 *   nginx 指令表    name + type + set + offset     —— 6 个字段，1 个函数指针
 *   redis 命令表    30+ 字段，其中只有 1 个是函数指针，其余全是元数据
 *
 *   也就是说：这张表不只在回答「谁来处理」，它同时回答
 *   「怎么校验」「怎么分类」「怎么文档化」「哪些 key 要转发」。
 *   一张表 = 多个消费方的单一事实源（single source of truth）。
 *
 * 本程序用一张表驱动 6 个消费方，并量化「不用表」的代价。
 * 编译：g++ -std=c++17 -Wall 03_meta_table.c -o /tmp/a05/03
 */

#include <cstdio>
#include <cstring>

/* ============ 表项：压缩版的 redisCommand（保留 6 个有代表性的字段） ============ */
enum { CMD_WRITE = 1u << 0, CMD_READONLY = 1u << 1, CMD_FAST = 1u << 2 };

typedef struct {
    const char *name;
    int (*proc)(int argc, const char **argv);
    int         arity;      /* redis 惯例：负数表示「至少 |arity| 个」（没有 arg 时用 0） */
    unsigned    flags;
    const char *group;
    const char *summary;
    int         firstkey;   /* cluster 路由用：第一个 key 的位置，0 表示无 key */
} redis_cmd_t;

/* ---------- 命令实现（占位） ---------- */
static int cmd_set (int, const char **) { std::printf("    [执行] SET\n");    return 1; }
static int cmd_get (int, const char **) { std::printf("    [执行] GET\n");    return 1; }
static int cmd_del (int, const char **) { std::printf("    [执行] DEL\n");    return 1; }
static int cmd_lpush(int, const char **) { std::printf("    [执行] LPUSH\n");  return 1; }

/* ============ 唯一的一份事实源 ============ */
static const redis_cmd_t commandTable[] = {
    { "set",   cmd_set,   -3, CMD_WRITE,                  "string", "Set the string value of a key", 1 },
    { "get",   cmd_get,    2, CMD_READONLY | CMD_FAST,    "string", "Get the value of a key",        1 },
    { "del",   cmd_del,   -2, CMD_WRITE,                  "generic","Delete a key",                  1 },
    { "lpush", cmd_lpush, -3, CMD_WRITE | CMD_FAST,       "list",   "Prepend an element to a list",  1 },
    { NULL,    NULL,        0, 0,                         NULL,     NULL,                            0 },
};
static int table_size(void) { int n = 0; while (commandTable[n].name) ++n; return n; }

/* ================= 消费方 1：执行分派 ================= */
static const redis_cmd_t *lookup(const char *name) {
    /* 真实 redis 用 dictFetchValue(server.commands, ...)，O(1)；这里小表用线性 */
    for (int i = 0; commandTable[i].name; ++i)
        if (std::strcmp(commandTable[i].name, name) == 0) return &commandTable[i];
    return NULL;
}

/* ================= 消费方 2：参数校验（arity） ================= */
static int check_arity(const redis_cmd_t *c, int argc) {
    if (c->arity >= 0) return argc == c->arity;
    return argc >= -c->arity;                    /* 负数 = 至少 N 个 */
}

/* ================= 消费方 3：COMMAND COUNT ================= */
static int command_count(void) { return table_size(); }

/* ================= 消费方 4：COMMAND DOCS ================= */
static void command_docs(void) {
    for (int i = 0; commandTable[i].name; ++i)
        std::printf("    %-8s - %s\n", commandTable[i].name, commandTable[i].summary);
}

/* ================= 消费方 5：分类统计（COMMAND LIST BY GROUP） ================= */
static void group_stats(void) {
    const char *seen[8]; int n = 0;
    for (int i = 0; commandTable[i].name; ++i) {
        const char *g = commandTable[i].group;
        int found = 0;
        for (int j = 0; j < n; ++j) if (std::strcmp(seen[j], g) == 0) found = 1;
        if (!found) seen[n++] = g;
    }
    for (int j = 0; j < n; ++j) {
        int cnt = 0;
        for (int i = 0; commandTable[i].name; ++i)
            if (std::strcmp(commandTable[i].group, seen[j]) == 0) ++cnt;
        std::printf("    %-8s : %d 条\n", seen[j], cnt);
    }
}

/* ================= 消费方 6：ACL 只读通道 / 集群 key 定位 ================= */
static void acl_and_cluster(void) {
    std::printf("    ACL 只读命令 :");
    for (int i = 0; commandTable[i].name; ++i)
        if (commandTable[i].flags & CMD_READONLY) std::printf(" %s", commandTable[i].name);
    std::printf("\n    有 key 的命令 :");
    for (int i = 0; commandTable[i].name; ++i)
        if (commandTable[i].firstkey > 0) std::printf(" %s", commandTable[i].name);
    std::printf("\n");
}

int main(void) {
    std::printf("=== 一张命令表，6 个消费方 ===\n\n");
    std::printf("表规模：%d 条 × 7 字段（真实 redis 是 30+ 字段 / %d 条）\n\n",
                table_size(), 464);

    std::printf("[1] 执行分派 lookup(\"lpush\")\n");
    const redis_cmd_t *c = lookup("lpush");
    if (c && check_arity(c, 3)) c->proc(3, NULL);

    std::printf("\n[2] 参数校验 check_arity\n");
    std::printf("    SET a b  (argc=3)  -> %s\n", check_arity(lookup("set"), 3) ? "OK" : "WRONGTYPE");
    std::printf("    GET a    (argc=2)  -> %s\n", check_arity(lookup("get"), 2) ? "OK" : "ERR");
    std::printf("    GET      (argc=1)  -> %s\n", check_arity(lookup("get"), 1) ? "OK" : "ERR");

    std::printf("\n[3] COMMAND COUNT\n    %d\n", command_count());

    std::printf("\n[4] COMMAND DOCS\n");
    command_docs();

    std::printf("\n[5] 按 group 统计\n");
    group_stats();

    std::printf("\n[6] ACL / 集群路由（同一张表再切两个维度）\n");
    acl_and_cluster();

    std::printf("\n=== 不用表会怎样（量化）===\n");
    std::printf("  这 %d 条命令，6 个消费方若各自维护一份知识 = %d 处需要同步\n",
                table_size(), table_size() * 6);
    std::printf("  用表：%d 行表项 + 6 个通用遍历器（遍历器不认识任何具体命令）\n",
                table_size());
    std::printf("  新增一条命令：表里加 1 行，6 个视图全部自动跟上\n");
    return 0;
}

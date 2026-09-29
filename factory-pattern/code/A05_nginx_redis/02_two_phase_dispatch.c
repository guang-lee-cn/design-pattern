/*
 * 02_two_phase_dispatch.c —— nginx 事件后端选择：把「名字」在配置期换成「整数索引」
 *
 * 出处：nginx 1.31.7
 *   src/event/ngx_event.c:1100-1136  配置解析：字符串比较后 ecf->use = modules[m]->ctx_index
 *   src/event/ngx_event.c:680-698     启动：遍历 modules 找 ctx_index == ecf->use
 *   src/event/modules/ngx_epoll_module.c:369
 *                                      ngx_event_actions = ngx_epoll_module_ctx.actions;
 *                                      ← 一次结构体整体赋值，搬走 9 个函数指针
 *   src/event/ngx_event.h:400-408      #define ngx_add_event ngx_event_actions.add
 *                                      ← 宏把「间接调用」伪装成普通函数名
 *
 * 本程序测的是「调用契约」的成本差异 —— 同一件事，调用方拿什么来换到函数：
 *   A. 契约是名字（字符串）  → 每次调用都要查找
 *   B. 契约是索引（整数）    → 直接索引表
 *   C. 契约是函数指针（已固化到全局）→ 直接解引用
 * nginx 的两阶段做的事，就是把契约从 A 降级到 C。
 *
 * 编译：g++ -std=c++17 -O2 -Wall 02_two_phase_dispatch.c -o /tmp/a05/02
 */

#include <cstdio>
#include <cstring>
#include <chrono>

/* ============ 后端接口（对应 ngx_event_actions_t，取 2 个字段示意） ============ */
struct actions_t {
    const char *name;
    int (*add)(int fd);
    int (*process)(int timeout_ms);
};

static int epoll_add(int)  { return 1; }
static int epoll_proc(int) { return 3; }
static int kqueue_add(int) { return 4; }
static int kqueue_proc(int){ return 6; }
static int select_add(int) { return 7; }
static int select_proc(int){ return 9; }

/* 静态表（真实 nginx 里由 auto/modules 生成的 ngx_modules[] 提供） */
static const actions_t backend_tab[] = {
    { "epoll",  epoll_add,  epoll_proc  },
    { "kqueue", kqueue_add, kqueue_proc },
    { "select", select_add, select_proc },
};
static constexpr int BACKEND_N = sizeof(backend_tab) / sizeof(backend_tab[0]);

/*
 * volatile：阻止编译器把 strcmp 的结果提升到循环外。
 * 上一版没有它，测出 0.256 ns/op —— 那是不可能的数字，说明 strcmp 被优化掉了。
 * 教训与 06 节一致：反常的数字先怀疑测法。
 */
static const char *volatile g_want_name = "kqueue";
static volatile int         g_ctx_index = 1;

static volatile long g_sink = 0;

/* ---------- A. 契约是名字：每次调用都要字符串比较 ---------- */
static void body_A(long n) {
    long acc = 0;
    for (long i = 0; i < n; ++i) {
        const char *want = g_want_name;           /* 每次重新读，无法提升 */
        int idx = -1;
        for (int k = 0; k < BACKEND_N; ++k) {
            if (std::strcmp(backend_tab[k].name, want) == 0) { idx = k; break; }
        }
        acc += backend_tab[idx].add((int)i);
    }
    g_sink = acc;
}

/* ---------- B. 契约是索引：直接索引表 ---------- */
static void body_B(long n) {
    long acc = 0;
    for (long i = 0; i < n; ++i) {
        acc += backend_tab[g_ctx_index].add((int)i);
    }
    g_sink = acc;
}

/* ---------- C. 契约是函数指针：已固化到全局（nginx 的做法） ---------- */
static actions_t g_actions;                        /* ← 对应 ngx_event_actions */

static void body_C(long n) {
    /* 阶段一：只做一次 —— 结构体整体赋值，9 个函数指针一次搬完 */
    g_actions = backend_tab[g_ctx_index];

    /* 阶段二：循环外取出函数指针，循环内零间接、零字符串 */
    int (*add)(int) = g_actions.add;
    long acc = 0;
    for (long i = 0; i < n; ++i) {
        acc += add((int)i);
    }
    g_sink = acc;
}

static double bench(const char *label, long n, void (*body)(long)) {
    body(n / 20);                                  /* 预热 */
    auto t0 = std::chrono::steady_clock::now();
    body(n);
    auto t1 = std::chrono::steady_clock::now();
    double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / (double)n;
    std::printf("  %-34s %8.3f ns/op\n", label, ns);
    return ns;
}

int main(void) {
    const long N = 20000000;

    std::printf("=== 调用契约的成本：名字 / 索引 / 函数指针（%ld 次）===\n\n", N);
    double a = bench("A. 契约=名字（每次 strcmp）",  N, body_A);
    double b = bench("B. 契约=索引（直接索引表）",   N, body_B);
    double c = bench("C. 契约=函数指针（固化全局）", N, body_C);

    std::printf("\n相对 A：B = %.2fx   C = %.2fx\n", a / b, a / c);
    std::printf("sink=%ld（防优化，非零即有效）\n", g_sink);

    std::printf("\n=== nginx 做两阶段换到了什么 ===\n");
    std::printf("阶段一（配置解析，一次性）：`use epoll;` -> 字符串比较 -> ecf->use = ctx_index\n");
    std::printf("阶段二（运行期，每次分派）：整数索引 -> 函数指针 -> 调用，零字符串操作\n");
    std::printf("ngx_event_actions_t 的 9 个指针：\n");
    std::printf("  add/del/enable/disable/add_conn/del_conn/notify/process_events/init/done\n");
    std::printf("一次 `ngx_event_actions = ngx_epoll_module_ctx.actions;` 全部搬完\n");
    return 0;
}

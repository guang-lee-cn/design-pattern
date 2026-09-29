/*
 * A04 / 02 —— C 版「抽象工厂」：一族函数指针表
 *
 * 与 C++ 版（05 节）最大的差异在这里：
 *   C++ 靠"选一个工厂对象"来表达产品族，类型系统帮不上太多但也给了个载体；
 *   C 没有类型系统可依靠 —— 族一致性**只能靠 API 形状保证**：
 *   入口只收"一张表"，混搭就根本写不出来。
 *
 * 换句话说：C++ 里约束来自"你只有一个工厂指针"，
 *           C  里约束来自"你只有一张表变量"。
 *
 * 编译：gcc -std=c11 -Wall -Wextra 02_family_table.c -o 02
 */
#include <stdio.h>
#include <string.h>

/* ================= 两个产品类型 ================= */

struct uart { int expect_gpio_base; };   /* 平台 A 的 UART 期望的 pinmux 位置 */
struct gpio { int base; };

/* 平台布局知识（调用方不该知道，也不该被允许混用） */
#define PLAT_A_GPIO_BASE 0x40001000
#define PLAT_B_GPIO_BASE 0x50002000

/* 演示用静态对象，省略生命周期管理（本节只讨论 API 形状） */

/* ================= 反例：三个独立工厂函数 ================= */

static struct uart *uart_create(const char *plat)
{
    static struct uart u;
    u.expect_gpio_base = (plat[0] == 'A') ? PLAT_A_GPIO_BASE : PLAT_B_GPIO_BASE;
    return &u;
}

static struct gpio *gpio_create(const char *plat)
{
    static struct gpio g;
    g.base = (plat[0] == 'A') ? PLAT_A_GPIO_BASE : PLAT_B_GPIO_BASE;
    return &g;
}

/* 绑定动作：跨平台必然对不上 */
static void uart_bind_gpio(struct uart *u, struct gpio *g)
{
    printf("    uart 期望 gpio base = 0x%08X\n", (unsigned)u->expect_gpio_base);
    printf("    gpio 实际 base      = 0x%08X\n", (unsigned)g->base);
    if (u->expect_gpio_base != g->base) {
        printf("    >> 平台不匹配：pinmux 写入错误寄存器组，UART 收不到数据\n");
    } else {
        printf("    >> 匹配，pinmux 配置生效\n");
    }
}

/* ================= 正例：一族一张表 ================= */

struct platform_ops {
    const char *hw;
    struct uart *(*uart_create)(void);
    struct gpio *(*gpio_create)(void);
};

static struct uart *uart_a(void)
{
    static struct uart u;
    u.expect_gpio_base = PLAT_A_GPIO_BASE;
    return &u;
}
static struct gpio *gpio_a(void)
{
    static struct gpio g;
    g.base = PLAT_A_GPIO_BASE;
    return &g;
}
static struct uart *uart_b(void)
{
    static struct uart u;
    u.expect_gpio_base = PLAT_B_GPIO_BASE;
    return &u;
}
static struct gpio *gpio_b(void)
{
    static struct gpio g;
    g.base = PLAT_B_GPIO_BASE;
    return &g;
}

static const struct platform_ops platform_a_ops = { "A", uart_a, gpio_a };
static const struct platform_ops platform_b_ops = { "B", uart_b, gpio_b };

/* 入口只收一张表 —— 内部两个产品必然同族 */
static void app_init(const struct platform_ops *plat)
{
    printf("    app_init(hw=%s)：两个产品都从这张表里取\n", plat->hw);
    uart_bind_gpio(plat->uart_create(), plat->gpio_create());
}

/* ================= 主程序 ================= */

int main(void)
{
    printf("=== 反例：三个独立工厂函数，各自收平台名 ===\n");
    printf("  [1] 同平台 A：\n");
    uart_bind_gpio(uart_create("A"), gpio_create("A"));

    printf("  [2] 混搭 A 的 uart + B 的 gpio（语法完全合法，编译器一声不吭）：\n");
    uart_bind_gpio(uart_create("A"), gpio_create("B"));

    printf("\n=== 正例：一族一张表，混搭无法表达 ===\n");
    printf("  [3] app_init(&platform_a_ops)：\n");
    app_init(&platform_a_ops);
    printf("  [4] app_init(&platform_b_ops)：\n");
    app_init(&platform_b_ops);

    printf("\n=== 结论 ===\n");
    printf("  同一份调用代码，换成'传表'之后，混搭不再是'记得不要做'，而是'写不出来'。\n");
    printf("  C 没有类型系统兜底，族约束只能落在参数形状上 —— 这是 C 版抽象工厂的关键设计点。\n");
    return 0;
}

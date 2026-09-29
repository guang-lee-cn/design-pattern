/*
 * A04 / 01 —— C 里怎么造工厂：手工 vtable + 两档工厂
 *
 * C 没有类、没有虚函数、没有模板。要在 C 里做到"同一接口、多种实现"，
 * 只剩一个原语可用：**结构体里放函数指针**（手工 vtable）。
 *
 * 于是 C 版工厂的形态是固定的：
 *   工厂函数 → 返回一个 { vptr, priv } 外壳，vptr 指向该产品的函数表。
 *
 * 编译：gcc -std=c11 -Wall -Wextra 01_vtable_and_factory.c -o 01
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ================= ① 手工 vtable：C 的唯一多态手段 ================= */

struct sensor;                        /* 不透明句柄：使用者看不到内部布局 */

struct sensor_ops {                   /* ≈ C++ 的虚函数表 */
    const char *name;
    int  (*read)(void *priv);         /* ≈ 虚函数（无 this，priv 手工传） */
    void (*destroy)(void *priv);      /* C 没有析构函数，销毁必须写成契约 */
};

struct sensor {                       /* ≈ 基类对象 */
    const struct sensor_ops *ops;     /* vptr */
    void *priv;                       /* 具体实现的状态（C++ 里是派生类成员） */
};

static int sensor_read(const struct sensor *s)
{
    return s->ops->read(s->priv);     /* 唯一的"虚调用"，手写一行 */
}

static void sensor_destroy(struct sensor *s)
{
    if (!s) return;
    s->ops->destroy(s->priv);         /* ≈ virtual ~Base()：先析构派生，再放外壳 */
    free(s);
}

/* ================= 具体产品 1：温度 ================= */

struct temp_priv { double calib; };

static int temp_read(void *priv)
{
    return (int)(((struct temp_priv *)priv)->calib * 10.0);
}

static void temp_destroy(void *priv)
{
    printf("    [temp]     释放实现私有状态\n");
    free(priv);
}

/* 每个产品一张表 —— 表本身就是"类型" */
static const struct sensor_ops temp_ops = { "temp", temp_read, temp_destroy };

/* 构造函数：C 里"造对象"就是一个函数，没有 new 表达式 */
static struct sensor *temp_create(void)
{
    struct sensor *s = malloc(sizeof *s);
    struct temp_priv *p = malloc(sizeof *p);
    if (!s || !p) { free(s); free(p); return NULL; }
    p->calib = 25.0;
    s->ops = &temp_ops;
    s->priv = p;
    return s;
}

/* ================= 具体产品 2：压力 ================= */

struct press_priv { int range; };

static int press_read(void *priv)
{
    return ((struct press_priv *)priv)->range;
}

static void press_destroy(void *priv)
{
    printf("    [pressure] 释放实现私有状态\n");
    free(priv);
}

static const struct sensor_ops press_ops = { "pressure", press_read, press_destroy };

static struct sensor *press_create(void)
{
    struct sensor *s = malloc(sizeof *s);
    struct press_priv *p = malloc(sizeof *p);
    if (!s || !p) { free(s); free(p); return NULL; }
    p->range = 110;                   /* 产品决策：默认量程，调用方不需要知道 */
    s->ops = &press_ops;
    s->priv = p;
    return s;
}

/* ================= 档 1：字符串分派（= 03 节简单工厂） ================= */

static struct sensor *sensor_create_switch(const char *type)
{
    if (strcmp(type, "temp") == 0)     return temp_create();
    if (strcmp(type, "pressure") == 0) return press_create();
    return NULL;                      /* C 没有异常，错误只能靠返回值 */
}

/* ================= 档 2：函数指针表（= 04a 节注册表，但更轻） ================= */

typedef struct sensor *(*sensor_ctor)(void);   /* 只是"函数指针"，没有类型擦除 */

struct sensor_entry {
    const char *key;
    sensor_ctor ctor;
};

static const struct sensor_entry registry[] = {
    { "temp",     temp_create  },
    { "pressure", press_create },
};

static struct sensor *sensor_create_table(const char *type)
{
    const size_t n = sizeof(registry) / sizeof(registry[0]);
    for (size_t i = 0; i < n; ++i)
        if (strcmp(registry[i].key, type) == 0)
            return registry[i].ctor();
    return NULL;
}

/* ================= 主程序 ================= */

static void demo(struct sensor *(*make)(const char *), const char *label, const char *type)
{
    struct sensor *s = make(type);
    if (!s) {
        printf("  %-22s %-9s -> NULL（未注册类型）\n", label, type);
        return;
    }
    printf("  %-22s %-9s -> read=%-4d ops=%s\n", label, type, sensor_read(s), s->ops->name);
    sensor_destroy(s);
}

int main(void)
{
    printf("=== C 版手工 vtable 的尺寸（64 位）===\n");
    printf("  sizeof(struct sensor_ops)   = %2zu  ← name + 2 个函数指针\n",
           sizeof(struct sensor_ops));
    printf("  sizeof(struct sensor)       = %2zu  ← vptr + priv（两个指针）\n",
           sizeof(struct sensor));
    printf("  sizeof(struct sensor_entry) = %2zu  ← 注册表表项：key + 函数指针\n",
           sizeof(struct sensor_entry));

    printf("\n=== 档 1：字符串分派 ===\n");
    demo(sensor_create_switch, "sensor_create_switch", "temp");
    demo(sensor_create_switch, "sensor_create_switch", "pressure");

    printf("\n=== 档 2：函数指针表 ===\n");
    demo(sensor_create_table, "sensor_create_table", "temp");
    demo(sensor_create_table, "sensor_create_table", "pressure");

    printf("\n=== 未注册类型：C 与 C++ 的差别 ===\n");
    demo(sensor_create_switch, "switch", "humidity");
    demo(sensor_create_table,  "table ", "humidity");
    printf("  （C++ 版可以返回异常或 std::optional；C 版只有 NULL 或错误码，\n");
    printf("    调用方漏判 NULL 就是死机 —— 这是 C 工厂最贵的代价之一）\n");

    printf("\n=== 档 3：族表见 02_family_table.c；档 4：段自注册见 03/04 ===\n");
    return 0;
}

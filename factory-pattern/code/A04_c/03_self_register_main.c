/*
 * A04 / 03 —— C 独有的一档：链接器段自注册
 *
 * 04a 节那套注册表要有人显式调 register_xxx()（"装配点"）：
 *   主程序仍要知道每个产品，才能把它登记进来。
 *
 * 这里连这步都省了：注册发生在【静态初始化期】—— 把表项放进一个自定义段，
 * 运行期用链接器自动生成的 __start_/__stop_ 符号遍历。
 *   新增产品 = 新增一个 .c 文件；主程序与任何 central list 都不动。
 *
 * 这不是玩具：Linux 内核的 module_init() / initcall 就是这个机制
 * （见内核头文件 include/linux/init.h 的 __define_initcall）。
 *
 * 编译方式见 build.sh（本节要配合 04_self_register_gyro.c 做链接实验）
 */
#include <stdio.h>
#include <string.h>

#include "sensor_registry.h"

/* ---- 本文件登记两个产品：一条宏，零装配代码 ---- */

static int temp_read(void)  { return 250; }
static int press_read(void) { return 110; }

REGISTER_DRIVER("temp",     temp_read);
REGISTER_DRIVER("pressure", press_read);

/* ---- 按名查表：遍历段即可，不需要任何"注册表对象" ---- */

static int lookup(const char *key)
{
    for (const struct drv_entry *p = __start_sensor_registry;
         p < __stop_sensor_registry; ++p) {
        if (strcmp(p->key, key) == 0)
            return p->read();
    }
    return -1;                        /* 未注册 */
}

int main(void)
{
    printf("=== 段自注册：段内登记项 %zu 个 ===\n", sensor_registry_count());
    for (const struct drv_entry *p = __start_sensor_registry;
         p < __stop_sensor_registry; ++p) {
        printf("    %-10s -> %d\n", p->key, p->read());
    }

    printf("\n=== 按名查表（主程序不认识任何具体产品）===\n");
    printf("    lookup(\"temp\")  = %3d\n", lookup("temp"));
    printf("    lookup(\"press\") = %3d\n", lookup("pressure"));
    printf("    lookup(\"gyro\")  = %3d   ← 该驱动登记在静态库成员里\n", lookup("gyro"));
    printf("    lookup(\"nope\")  = %3d   ← 未注册类型\n", lookup("nope"));

    printf("\n=== 链接方式会改变段内容：见 build.sh 的 A / B 两种链接 ===\n");
    return 0;
}

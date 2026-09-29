/*
 * A04 · 链接器段自注册：段内表项的公共声明
 *
 * 主程序（03_self_register_main.c）与库成员（04_self_register_gyro.c）
 * 都通过本头文件看到同一份布局，避免两个 TU 各写一份 struct 定义。
 */
#ifndef SENSOR_REGISTRY_H
#define SENSOR_REGISTRY_H

#include <stddef.h>

/* 表项：一个键 + 一个创建/读取函数指针（没有任何"工厂类"） */
struct drv_entry {
    const char *key;
    int (*read)(void);
};

/*
 * 把表项塞进名为 sensor_registry 的自定义段。
 * GNU ld 会为"合法 C 标识符"的段名自动生成 __start_<sec> / __stop_<sec> 符号，
 * 运行期据此遍历 —— 这就是内核 initcall 机制的最小复刻。
 */
#define REGISTER_DRIVER(key_, fn_)                                       \
    static const struct drv_entry                                        \
        __attribute__((used, section("sensor_registry"),                  \
                       aligned(sizeof(void *))))                         \
        reg_entry_##fn_ = { (key_), (fn_) }

/* 链接器自动生成的段边界符号 */
extern const struct drv_entry __start_sensor_registry[];
extern const struct drv_entry __stop_sensor_registry[];

static inline size_t sensor_registry_count(void)
{
    return (size_t)(__stop_sensor_registry - __start_sensor_registry);
}

#endif /* SENSOR_REGISTRY_H */

/* plugin_abi.h —— 宿主与插件之间的唯一契约
 *
 * 三条规则，缺一不可：
 *   1. 只允许出现 C 类型。没有 std::string、没有虚函数、没有模板、没有异常跨边界。
 *   2. 入口符号名固定，且必须 extern "C" —— C++ 会做名字修饰，dlsym 按原名的拿不到。
 *   3. 每次改动下面这个结构体的布局，PLUGIN_ABI_VERSION 必须 +1；
 *      宿主再校验一次 struct_size，兜住"忘了改版本号"。
 *
 * 三规则分别对应 A06 的三个实测：符号协议 / 双校验 / 卸载。
 */
#ifndef PLUGIN_ABI_H
#define PLUGIN_ABI_H

#include <stdint.h>
#include <stddef.h>

/* 版本号是"约定"：靠人记得改。struct_size 是"机制"：不靠人记。 */
#define PLUGIN_ABI_VERSION 3u

#if defined(_WIN32)
#  define PLUGIN_EXPORT __declspec(dllexport)
#else
/* 配合 -fvisibility=hidden 使用：只把标了 PLUGIN_EXPORT 的符号放进动态符号表 */
#  define PLUGIN_EXPORT __attribute__((visibility("default")))
#endif

/* 插件入口返回的描述块。宿主只认这一种布局。 */
typedef struct PluginDesc {
    uint32_t    abi_version;   /* 必须 == PLUGIN_ABI_VERSION          */
    uint32_t    struct_size;   /* 必须 == sizeof(PluginDesc) —— 第二道锁 */
    const char* name;
    int       (*read)(void* self);
    void*       self;
} PluginDesc;

/* 唯一导出符号的类型：入参是宿主自己的版本号，插件也可据此拒绝加载 */
typedef const PluginDesc* (*plugin_entry_fn)(uint32_t host_abi_version);

#define PLUGIN_ENTRY_SYMBOL "plugin_entry"

#endif /* PLUGIN_ABI_H */

// rtti_host.cpp —— 宿主：跨 .so 做 dynamic_cast，看能不能认出来
//
// 这是插件的经典坑：动态库若用 -fvisibility=hidden 编译，
// 它那一份 typeinfo 不会进动态符号表，宿主手上那份就不会被它替换掉。
#include "rtti_iface.h"
#include <dlfcn.h>
#include <cstdio>
#include <cstring>
#include <typeinfo>

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("用法: rtti_host <插件.so> [unload]\n");
        return 2;
    }
    const char* path = argv[1];

    void* h = dlopen(path, RTLD_NOW | RTLD_LOCAL);
    if (!h) {
        std::printf("[rtti] dlopen 失败: %s\n", dlerror());
        return 1;
    }

    typedef Sensor* (*make_fn)();
    auto make = reinterpret_cast<make_fn>(dlsym(h, "make_sensor"));
    if (!make) {
        std::printf("[rtti] dlsym(make_sensor) 失败: %s\n", dlerror());
        dlclose(h);
        return 1;
    }

    Sensor* s = make();
    std::printf("[rtti] 插件: %s\n", path);
    std::printf("[rtti] 对象 @ %p, s->read() = %d\n",
                static_cast<void*>(s), s->read());

    /* 1) 虚调用 —— 走对象自己的 vtable，一定能通 */
    /* 2) dynamic_cast —— 需要宿主与插件对「TempSensor 是什么」达成一致 */
    TempSensor* t = dynamic_cast<TempSensor*>(s);
    std::printf("[rtti] dynamic_cast<TempSensor*>(s) = %s\n",
                t ? "成功" : "**失败（返回 nullptr）**");
    if (t) {
        std::printf("[rtti]   -> t->value = %d\n", t->value);
    }

    /* 3) typeid 的名字比较 —— 只比名字，比 dynamic_cast 宽松 */
    std::printf("[rtti] typeid(*s).name() = %s （期望 %s）\n",
                typeid(*s).name(), typeid(TempSensor).name());
    std::printf("[rtti] typeid(*s) == typeid(TempSensor) : %s\n",
                (typeid(*s) == typeid(TempSensor)) ? "true" : "false");
    std::printf("[rtti] &typeid(*s) = %p, &typeid(TempSensor) = %p （地址是否同一份）\n",
                static_cast<const void*>(&typeid(*s)),
                static_cast<const void*>(&typeid(TempSensor)));

    if (argc > 2 && std::strcmp(argv[2], "unload") == 0) {
        // 卸载后宿主手里还攥着这个对象 —— Linux 上 RTTI 真正会崩的那条路
        dlclose(h);
        std::printf("[rtti] dlclose 完成，宿主手里还攥着那个 Sensor*\n");
        std::fflush(stdout);
        std::printf("[rtti] 卸载后 typeid(*s).name() ...\n");
        std::fflush(stdout);
        const char* n = typeid(*s).name();
        std::printf("[rtti]   -> %s   <-- 没崩说明 typeinfo 还在\n", n);
        std::fflush(stdout);
        std::printf("[rtti] 卸载后 s->read() ...\n");
        std::fflush(stdout);
        std::printf("[rtti]   -> %d\n", s->read());
        return 0;
    }

    delete s;
    dlclose(h);
    return 0;
}

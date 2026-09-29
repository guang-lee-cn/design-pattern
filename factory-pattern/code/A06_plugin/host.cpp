// host.cpp —— 宿主：装载 / 校验 / 调用 / 卸载
//
//   host sizes                    打印宿主侧布局（offsetof / sizeof）
//   host load <so>                双校验（version + struct_size）后调用 read()
//   host version-only <so>        只校验 version —— 复现"忘了改版本号"的后果
//   host unload-use <so>          dlopen -> 用一次 -> dlclose -> 再用一次
//   host unload-nodelete <so>     同上，但 dlopen 带 RTLD_NODELETE
#include "plugin_abi.h"
#include <dlfcn.h>
#include <cstdio>
#include <string_view>

namespace {

const char* g_path = nullptr;

void* open_lib(int flags) {
    void* h = dlopen(g_path, flags);
    if (!h) {
        std::printf("[host] dlopen 失败: %s\n", dlerror());
    }
    return h;
}

plugin_entry_fn get_entry(void* h) {
    dlerror();                                     // 先清一次，否则读到的是旧错误
    auto fn = reinterpret_cast<plugin_entry_fn>(dlsym(h, PLUGIN_ENTRY_SYMBOL));
    if (const char* err = dlerror()) {
        std::printf("[host] dlsym(\"%s\") 失败: %s\n", PLUGIN_ENTRY_SYMBOL, err);
        return nullptr;
    }
    return fn;
}

int mode_sizes() {
    std::printf("宿主侧 sizeof(PluginDesc) = %zu\n", sizeof(PluginDesc));
    std::printf("  offsetof(abi_version) = %zu\n", offsetof(PluginDesc, abi_version));
    std::printf("  offsetof(struct_size) = %zu\n", offsetof(PluginDesc, struct_size));
    std::printf("  offsetof(name)        = %zu\n", offsetof(PluginDesc, name));
    std::printf("  offsetof(read)        = %zu\n", offsetof(PluginDesc, read));
    std::printf("  offsetof(self)        = %zu\n", offsetof(PluginDesc, self));
    return 0;
}

int mode_load(bool check_size) {
    void* h = open_lib(RTLD_NOW | RTLD_LOCAL);
    if (!h) return 1;
    std::printf("[host] dlopen ok: %s\n", g_path);

    plugin_entry_fn entry = get_entry(h);
    if (!entry) { dlclose(h); return 1; }
    std::printf("[host] dlsym(\"%s\") ok -> %p\n", PLUGIN_ENTRY_SYMBOL, reinterpret_cast<void*>(entry));
    std::printf("[host] 宿主期望 : abi_version=%u  sizeof(PluginDesc)=%zu\n",
                PLUGIN_ABI_VERSION, sizeof(PluginDesc));

    const PluginDesc* d = entry(PLUGIN_ABI_VERSION);
    if (!d) {
        std::printf("[host] 插件自己拒绝了这次加载（它那侧检查没过）\n");
        dlclose(h);
        return 1;
    }
    std::printf("[host] 插件报告 : abi_version=%u  struct_size=%u\n", d->abi_version, d->struct_size);

    if (d->abi_version != PLUGIN_ABI_VERSION) {
        std::printf("[host] abi_version 不匹配 -> 拒绝\n");
        dlclose(h);
        return 1;
    }
    if (check_size && d->struct_size != sizeof(PluginDesc)) {
        std::printf("[host] struct_size 不匹配（%u != %zu）-> 拒绝\n",
                    d->struct_size, sizeof(PluginDesc));
        dlclose(h);
        return 1;
    }
    std::printf("[host] 校验通过 %s\n",
                check_size ? "【version + struct_size 双校验】" : "【*** 只校验 version ***】");

    // 先只读指针值，不调用 —— 让"读到了什么"这件事在崩溃前就打印出来
    std::printf("[host] 按宿主布局读到: read=%p  self=%p\n",
                reinterpret_cast<void*>(d->read), d->self);
    std::fflush(stdout);

    std::printf("[host] name=%s  read()=%d\n", d->name, d->read(d->self));
    dlclose(h);
    return 0;
}

int mode_unload_use(int flags) {
    void* h = open_lib(flags);
    if (!h) return 1;

    plugin_entry_fn entry = get_entry(h);
    if (!entry) { dlclose(h); return 1; }

    const PluginDesc* d = entry(PLUGIN_ABI_VERSION);
    if (!d) { dlclose(h); return 1; }

    std::printf("[host] 卸载前: name=%s  read()=%d  desc@%p\n",
                d->name, d->read(d->self), static_cast<const void*>(d));

    dlclose(h);
    std::printf("[host] dlclose 完成（flags=%s）\n",
                (flags & RTLD_NODELETE) ? "RTLD_NOW|RTLD_NODELETE" : "RTLD_NOW");
    std::printf("[host] 现在问一句：库都卸了，宿主手里那个 desc 还能用吗？\n");
    std::printf("[host] 先只打印指针值（不访问内存）: desc=%p\n", static_cast<const void*>(d));
    std::fflush(stdout);

    std::printf("[host] 再一次调用 d->read(d->self) ...\n");
    std::fflush(stdout);
    std::printf("[host] read() = %d  <-- 没崩就说明代码段还在\n", d->read(d->self));
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("用法: host <sizes|load|version-only|unload-use|unload-nodelete> [插件.so]\n");
        return 2;
    }
    const char* mode = argv[1];
    const bool need_path = (std::string_view(mode) != "sizes");
    if (need_path && argc < 3) {
        std::printf("用法: host %s <插件.so>\n", mode);
        return 2;
    }
    if (need_path) g_path = argv[2];

    std::string_view m(mode);
    if (m == "sizes")          return mode_sizes();
    if (m == "load")           return mode_load(true);
    if (m == "version-only")   return mode_load(false);
    if (m == "unload-use")     return mode_unload_use(RTLD_NOW | RTLD_LOCAL);
    if (m == "unload-nodelete")return mode_unload_use(RTLD_NOW | RTLD_LOCAL | RTLD_NODELETE);
    std::printf("未知模式: %s\n", mode);
    return 2;
}

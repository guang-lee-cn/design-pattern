// plugin_mangled.cpp —— 反例 1：入口函数忘了加 extern "C"
//
// 故意错误：编译、链接全过（.so 照样生成），只有宿主 dlsym 时会失败。
// 实测对比（build.sh 会打印 nm -D）：
//   plugin_temp.cpp  ->  plugin_entry
//   plugin_mangled.cpp -> _Z12plugin_entryj     （GCC/Itanium ABI 的名字修饰）
#include "plugin_abi.h"

namespace {

int mangled_read(void*) { return 999; }

const PluginDesc g_desc = {
    PLUGIN_ABI_VERSION,
    sizeof(PluginDesc),
    "mangled",
    &mangled_read,
    nullptr,
};

}  // namespace

// ↓↓↓ 这里少写了 extern "C"，符号名会被修饰成 _Z12plugin_entryj
PLUGIN_EXPORT const PluginDesc* plugin_entry(uint32_t host_abi_version) {
    if (host_abi_version != PLUGIN_ABI_VERSION) {
        return nullptr;
    }
    return &g_desc;
}

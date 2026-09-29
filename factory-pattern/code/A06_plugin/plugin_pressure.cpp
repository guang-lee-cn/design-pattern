// plugin_pressure.cpp —— 正常插件 B（压力）
//
// 与 plugin_temp.cpp 同构，只是数据不同：证明宿主对插件是"一视同仁"的，
// 加一个新插件不需要改宿主一行代码（这正是 A06 要买的东西）。
#include "plugin_abi.h"

namespace {

struct PressureSensor {
    int range_kpa;
};

int pressure_read(void* self) {
    PressureSensor* s = static_cast<PressureSensor*>(self);
    return 1013 + s->range_kpa;       // 101.3 kPa，放大 10 倍
}

PressureSensor g_sensor{0};

const PluginDesc g_desc = {
    PLUGIN_ABI_VERSION,
    sizeof(PluginDesc),
    "pressure",
    &pressure_read,
    &g_sensor,
};

}  // namespace

extern "C" PLUGIN_EXPORT const PluginDesc* plugin_entry(uint32_t host_abi_version) {
    if (host_abi_version != PLUGIN_ABI_VERSION) {
        return nullptr;
    }
    return &g_desc;
}

// plugin_temp.cpp —— 正常插件 A（温度）
//
// 除了入口符号，这里还故意留了几个「外部链接的实现细节」（真实项目里到处都是）：
// 它们本不该进插件的公开符号表，但**默认可见性下会进去**。见 build.sh 实验 2。
#include "plugin_abi.h"

/* 实现细节 1：外部链接，非 API —— 默认可见性下会被导出 */
int plugin_internal_gain(void) { return 2; }

/* 实现细节 2：同上 */
int plugin_internal_clamp(int v, int hi) { return v > hi ? hi : v; }

/* 实现细节 3：模板函数 —— 实例化出来的是 weak 符号，同样会被导出 */
template <typename T>
T plugin_internal_max(T a, T b) { return a > b ? a : b; }

namespace {

struct TempSensor {
    int calibration;
};

int temp_read(void* self) {
    TempSensor* s = static_cast<TempSensor*>(self);
    const int scaled = 235 + s->calibration * plugin_internal_gain();
    return plugin_internal_clamp(plugin_internal_max(scaled, 0), 4095);   // -> 235
}

TempSensor g_sensor{0};               // 插件私有数据：宿主只当它是 void*

const PluginDesc g_desc = {
    PLUGIN_ABI_VERSION,
    sizeof(PluginDesc),
    "temp",
    &temp_read,
    &g_sensor,
};

}  // namespace

// 唯一「该导出」的符号。extern "C" 保证符号名就是 plugin_entry（不做修饰）。
extern "C" PLUGIN_EXPORT const PluginDesc* plugin_entry(uint32_t host_abi_version) {
    if (host_abi_version != PLUGIN_ABI_VERSION) {
        return nullptr;               // 插件侧也守一道
    }
    return &g_desc;
}

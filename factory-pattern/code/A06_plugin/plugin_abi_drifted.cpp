// plugin_abi_drifted.cpp —— 反例 2：结构体布局变了，但（可能）忘了改版本号
//
// 同一份源码编出两个 .so：
//   不加 -DABI_BUMP  -> libabi_drifted.so  （版本号仍是 3，布局已变）← 事故
//   加   -DABI_BUMP  -> libabi_bumped.so   （版本号改成 4，布局已变）← 正常演进
//
// 这一份的「新布局」在 name 后面插了一个字段。宿主按旧布局读，会
// 把那个字段当成 read 函数指针去调用 —— 实测会跳到地址 0x64。
#include "plugin_abi.h"

/* 插件侧自认的布局（v3 的「演进版」） */
typedef struct PluginDescDrifted {
    uint32_t    abi_version;
    uint32_t    struct_size;
    const char* name;
    uint32_t    sample_rate_hz;   /* ← 新插入的字段，把后面所有字段整体后移 */
    int       (*read)(void* self);
    void*       self;
} PluginDescDrifted;

/* 故意把"采样率"设成一个一眼能认出来的小整数：宿主会拿它当函数地址调用 */
#define SAMPLE_RATE_HZ 100u

#ifdef ABI_BUMP
#  define REPORTED_VERSION 4u                    /* 纪律：改布局就改版本号 */
#else
#  define REPORTED_VERSION PLUGIN_ABI_VERSION    /* 事故：忘了改 */
#endif

namespace {

int drifted_read(void*) { return 777; }

PluginDescDrifted g_desc = {
    REPORTED_VERSION,
    sizeof(PluginDescDrifted)
#ifdef ABI_BUMP
    , "drifted_bumped"
#else
    , "drifted"
#endif
    , SAMPLE_RATE_HZ,
    &drifted_read,
    nullptr,
};

}  // namespace

extern "C" PLUGIN_EXPORT const PluginDesc* plugin_entry(uint32_t host_abi_version) {
    /* 插件只做"版本号相等"这一道检查 —— 因为它的版本号没改，所以会放行 */
    if (host_abi_version != REPORTED_VERSION) {
        return nullptr;
    }
    return reinterpret_cast<const PluginDesc*>(&g_desc);
}

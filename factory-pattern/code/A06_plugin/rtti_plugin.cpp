// rtti_plugin.cpp —— 在 .so 内部 new 一个 TempSensor，交给宿主
//
// 同一份源码编两次：
//   默认可见性            -> librtti_vis.so
//   -fvisibility=hidden   -> librtti_hidden.so（只导出 make_sensor）
#include "rtti_iface.h"
#include "plugin_abi.h"

extern "C" PLUGIN_EXPORT Sensor* make_sensor() {
    return new TempSensor{42};
}

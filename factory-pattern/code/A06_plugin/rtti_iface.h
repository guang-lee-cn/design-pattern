// rtti_iface.h —— 只用于「跨 .so 的 RTTI」实验
//
// 宿主与插件都 include 这个头，于是双方各有一份 TempSensor 的
// vtable 与 typeinfo（weak 符号）。问题就在这两份是不是同一个。
#pragma once

struct Sensor {
    virtual ~Sensor() = default;
    virtual int read() const = 0;
};

/* 宿主也要知道 TempSensor 的完整定义，才能 dynamic_cast 到它 */
struct TempSensor : Sensor {
    int value = 0;

    TempSensor() = default;
    explicit TempSensor(int v) : value(v) {}

    int read() const override { return value; }
};

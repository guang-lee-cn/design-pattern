// 02_cost · 路线一「硬编码调用」—— 两个显示端，各自独立
//
// 它们没有公共基类：传感器直接认识具体类型（见 sensor.h），
// 所以这里不需要抽象接口。这既是路线一的省事之处，
// 也是它「加一个显示端就得改传感器」的根源。
#pragma once
#include <cstdio>

class NumberDisplay {
public:
    void show(double t) const {
        std::printf("[%5.1f] %-14s %6.1f C\n", t, "NumberDisplay", t);
    }
};

class BarDisplay {
public:
    void show(double t) const {
        int bars = static_cast<int>(t);
        std::printf("[%5.1f] %-14s ", t, "BarDisplay");
        for (int i = 0; i < bars; ++i)  std::printf("#");
        for (int i = bars; i < 40; ++i) std::printf("-");
        std::printf(" %d/40\n", bars);
    }
};

class AlertDisplay {
public:
    void show(double t) const {
        std::printf("[%5.1f] %-14s %s\n", t, "AlertDisplay", t > 24.0 ? "!! HIGH" : "ok");
    }
};

// 02_cost · 路线三「持有名单」—— 所有显示端的统一接口
//
// 传感器只认识 Display，不认识任何具体显示端（见 sensor.h）。
// 代价是多了一层间接：想加显示端，得先实现这个接口。
// 完整形态（四层谱系）在第 4 节展开，这里只取足以对照的最小形式。
#pragma once
#include <cstdio>

class Display {
public:
    virtual ~Display() = default;
    virtual void show(double t) = 0;
};

class NumberDisplay : public Display {
public:
    void show(double t) override {
        std::printf("[%5.1f] %-14s %6.1f C\n", t, "NumberDisplay", t);
    }
};

class BarDisplay : public Display {
public:
    void show(double t) override {
        int bars = static_cast<int>(t);
        std::printf("[%5.1f] %-14s ", t, "BarDisplay");
        for (int i = 0; i < bars; ++i)  std::printf("#");
        for (int i = bars; i < 40; ++i) std::printf("-");
        std::printf(" %d/40\n", bars);
    }
};

// fanout · 显示端接口与实现（与 observer/display.h 同源）
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

// fanout · 传感器（持有名单版，与 observer/sensor.h 同源）
//
// 这个头文件被 4 个翻译单元直接 #include —— 这正是「公共类」的含义，
// 也是「改动落在它身上」代价的放大器。
#pragma once
#include <algorithm>
#include <vector>
#include "display.h"

class TempSensor {
public:
    void attach(Display* d) { viewers_.push_back(d); }
    void detach(Display* d) {
        viewers_.erase(std::remove(viewers_.begin(), viewers_.end(), d), viewers_.end());
    }

    void set(double t) {
        t_ = t;
        for (Display* v : viewers_) v->show(t_);
    }
    double get() const { return t_; }

private:
    double                t_ = 0.0;
    std::vector<Display*> viewers_;
};

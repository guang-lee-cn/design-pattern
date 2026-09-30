// 路线三：传感器只持有一张「名单」，值一变就对名单里的每个对象说一声。
// ⇒ 新增显示端不用改这个文件：名单里装的是 Display*，不是具体类型。
#pragma once
#include <algorithm>
#include <vector>
#include "display.h"

class TempSensor {
public:
    void attach(Display* d) { viewers_.push_back(d); }   // 登记
    void detach(Display* d) {                            // 注销
        viewers_.erase(std::remove(viewers_.begin(), viewers_.end(), d), viewers_.end());
    }

    void set(double t) {
        t_ = t;
        for (Display* v : viewers_) v->show(t_);   // ← 一句话，与名单长度无关
    }
    double get() const { return t_; }

private:
    double                t_ = 0.0;
    std::vector<Display*> viewers_;   // ← 名单
};

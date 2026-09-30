// 路线一：传感器把两个显示端当成员直接持有，值一变就逐个点名调用。
// ⇒ 新增第三个显示端，必须回到这个文件里加成员和调用点。
#pragma once
#include "display.h"

class TempSensor {
public:
    void set(double t) {
        t_ = t;
        number_.show(t_);   // ← 调用点 ①
        bar_.show(t_);      // ← 调用点 ②
    }
    double get() const { return t_; }

private:
    double        t_ = 0.0;
    NumberDisplay number_;   // ← 具体类型成员 ①
    BarDisplay    bar_;      // ← 具体类型成员 ②
};

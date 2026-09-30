// 路线三的调用方：谁来显示、按什么顺序显示，在这里组装。
// 这个位置就是第 6 节要展开的「组合根」——注册订阅的调用处。
#include "display.h"
#include "sensor.h"

int main() {
    TempSensor sensor;

    NumberDisplay number;
    BarDisplay    bar;
    AlertDisplay  alert;      // ← 新增：第 3 个显示端
    sensor.attach(&number);   // ← 登记
    sensor.attach(&bar);      // ← 登记
    sensor.attach(&alert);    // ← 登记

    const double timeline[] = {23.5, 24.1, 25.0};
    for (double t : timeline) sensor.set(t);
    return 0;
}

// 路线一的调用方：它什么都不用做 —— 「谁来显示」是传感器自己的事。
// 这句话反过来读就是代价：调用方也管不了传感器认识谁。
#include "sensor.h"

int main() {
    TempSensor sensor;
    const double timeline[] = {23.5, 24.1, 25.0};
    for (double t : timeline) sensor.set(t);
    return 0;
}

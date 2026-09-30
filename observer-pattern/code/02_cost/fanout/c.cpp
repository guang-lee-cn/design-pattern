// fanout / 业务模块 C：采样计数（不参与显示，只是另一个依赖传感器头的模块）
#include <cstdio>
#include "sensor.h"

void module_c(const TempSensor& s) {
    static int n = 0;
    ++n;
    std::printf("[module C] sample #%d = %.1f C\n", n, s.get());
}

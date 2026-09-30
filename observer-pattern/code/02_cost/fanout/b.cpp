// fanout / 业务模块 B：越限检查
#include <cstdio>
#include "sensor.h"

void module_b(const TempSensor& s) {
    std::printf("[module B] over-limit check: %s\n", s.get() > 24.0 ? "ALARM" : "ok");
}

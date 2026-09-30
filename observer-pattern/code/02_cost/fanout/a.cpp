// fanout / 业务模块 A：把当前读数写进日志
#include <cstdio>
#include "sensor.h"

void module_a(const TempSensor& s) {
    std::printf("[module A] log: %.1f C\n", s.get());
}

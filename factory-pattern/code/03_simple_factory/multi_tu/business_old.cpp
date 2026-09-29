// 旧世界对照组：业务 TU 自己 include 具体类头 + 自己 if-else new
// 与 business.cpp 做「同一份业务逻辑，两种依赖面」的对照
#include "pressure_sensor.h"
#include "temp_sensor.h"

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

std::unique_ptr<Sensor> create_sensor_old(const std::string& type) {
    if (type == "temp")     return std::make_unique<TempSensor>();
    if (type == "pressure") return std::make_unique<PressureSensor>();
    throw std::invalid_argument("unknown sensor type: " + type);
}

void report_old(const std::string& type) {
    auto s = create_sensor_old(type);
    std::printf("read: %s\n", s->read().c_str());
}

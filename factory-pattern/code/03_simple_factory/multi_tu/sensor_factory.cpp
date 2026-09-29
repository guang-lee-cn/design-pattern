// ★ 唯一耦合点：全项目只有这个 TU include 具体传感器头
// 新增产品只改这里；具体传感器的头文件改动只触发这里重编
#include "sensor_factory.h"

#include "pressure_sensor.h"
#include "temp_sensor.h"

#include <stdexcept>

std::unique_ptr<Sensor> create_sensor(const std::string& type) {
    if (type == "temp")     return std::make_unique<TempSensor>();
    if (type == "pressure") return std::make_unique<PressureSensor>();
    throw std::invalid_argument("unknown sensor type: " + type);
}

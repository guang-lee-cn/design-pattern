// 具体产品头：只有工厂 TU 该看到它
#pragma once

#include "sensor.h"

class PressureSensor : public Sensor {
public:
    std::string read() const override;
};

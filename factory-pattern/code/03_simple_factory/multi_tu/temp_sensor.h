// 具体产品头：只有工厂 TU 该看到它
#pragma once

#include "sensor.h"

class TempSensor : public Sensor {
public:
    std::string read() const override;
};

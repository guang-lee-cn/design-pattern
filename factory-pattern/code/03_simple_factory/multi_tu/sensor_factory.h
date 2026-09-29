// 工厂头：业务侧只 include 这一个 + 抽象产品头
// 声明与实现分离——头文件里看不到任何具体类名
#pragma once

#include <memory>
#include <string>

#include "sensor.h"

std::unique_ptr<Sensor> create_sensor(const std::string& type);

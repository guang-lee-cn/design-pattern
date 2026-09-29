// 抽象产品头：业务侧唯一"该依赖"的产品头
// 它不含任何具体类、不含任何构造参数知识——所以它的改动不影响业务语义
#pragma once

#include <string>

class Sensor {
public:
    virtual ~Sensor() = default;
    virtual std::string read() const = 0;
};

// 新世界：业务 TU 只认识「工厂签名」和「抽象产品」
// 依赖列表里没有 temp_sensor.h / pressure_sensor.h —— 这就是痛点 1 的解
#include "sensor_factory.h"

#include <cstdio>

void report(const std::string& type) {
    // 没有具体类名，没有构造参数知识，没有 if-else
    auto s = create_sensor(type);
    std::printf("read: %s\n", s->read().c_str());
}

// 01-诞生背景：old.cpp —— 调用方直接 new 的写法
// 编译：g++ -std=c++17 -Wall old.cpp -o old && ./old

#include <cstdio>
#include <memory>
#include <string>

// ---------- 产品体系（假设来自某个传感器驱动库） ----------

class Sensor {
public:
    virtual ~Sensor() = default;
    virtual std::string read() = 0;
};

class TempSensor : public Sensor {
public:
    std::string read() override { return "23.5C"; }
};

class PressureSensor : public Sensor {
public:
    std::string read() override { return "101.3kPa"; }
};

// ---------- 调用方：业务代码 ----------

// 痛点 1：这里必须 #include TempSensor/PressureSensor 的完整定义，
//         业务代码和具体传感器实现焊死。
// 痛点 2：换传感器？新增传感器？每个 new 的地方都要改。
void report(const std::string& type) {
    std::unique_ptr<Sensor> s;
    if (type == "temp") {
        s = std::make_unique<TempSensor>();      // <-- 直接 new 具体类
    } else if (type == "pressure") {
        s = std::make_unique<PressureSensor>();  // <-- 又一处
    }
    if (s) {
        std::printf("read: %s\n", s->read().c_str());
    }
}

int main() {
    report("temp");
    report("pressure");
    return 0;
}

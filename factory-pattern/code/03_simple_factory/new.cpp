// 03-简单工厂：new.cpp —— 对照 01_birth/old.cpp，引入简单工厂
// 编译：g++ -std=c++17 -Wall new.cpp -o new && ./new
//
// 对照对象：code/01_birth/old.cpp（业务函数里直接 if-else new 具体类）
// 本例改动：创建决策收进 create_sensor() 一个函数，业务侧只剩抽象。

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

// ---------- 产品体系（与 old.cpp 相同） ----------

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

// ---------- 简单工厂：全部创建决策的唯一居所 ----------
// 第 1 节三个痛点在此的解法：
//   痛点 1（编译耦合）：业务 TU 只 include 工厂头文件（本文件顶部
//     的产品定义只被工厂 TU include，见下方"真实项目布局"注释）
//   痛点 2（修改扩散）：新增传感器只改这一个函数
//   痛点 3（构造知识泄漏）：TempSensor 的校准参数、PressureSensor 的
//     默认量程，全部封在这里，业务侧无感

std::unique_ptr<Sensor> create_sensor(const std::string& type) {
    if (type == "temp")          return std::make_unique<TempSensor>();
    if (type == "pressure")      return std::make_unique<PressureSensor>();
    throw std::invalid_argument("unknown sensor type: " + type);
}

// ---------- 业务代码：只认识抽象 Sensor ----------

void report(const std::string& type) {
    // 没有具体类名，没有构造参数知识，没有 if-else
    auto s = create_sensor(type);
    std::printf("read: %s\n", s->read().c_str());
}

// 真实项目布局（单文件演示无法体现，注释说明）：
//   sensor_factory.h   —— 只声明 create_sensor()，业务侧只 include 这个
//   sensor_factory.cpp —— include 全部具体传感器头文件，是唯一耦合点
//   business.cpp       —— include sensor_factory.h，编译期与具体类解耦
//
// 业务 TU 编译期不再依赖 TempSensor/PressureSensor 的头文件，
// 具体传感器头文件改动时，business.cpp 不重编——痛点 1 的解。

int main() {
    report("temp");
    report("pressure");
    try {
        report("humidity");  // 未注册的类型：工厂是唯一报错点，报错信息集中
    } catch (const std::exception& e) {
        std::printf("expected error: %s\n", e.what());
    }
    return 0;
}

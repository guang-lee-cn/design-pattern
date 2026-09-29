// 02-三个层次：pain_of_switch.cpp —— "switch 体频繁变更伤人"的具体形态
// 编译：g++ -std=c++17 -Wall pain_of_switch.cpp -o pain && ./pain
//
// 演示三层伤害（对照 docs/02-三个层次.md 升级判据 1）：
//   伤 A：已测试代码体被反复编辑（回归风险，java9r: "editing the already-tested body"）
//   伤 B：分支间隐式耦合，改一处牵连另一处（新分支必须记得做的"仪式"越来越多）
//   伤 C：并行改动合并冲突（两个新传感器同时开发，都改同一个 switch）

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

// ---------- 产品体系 ----------

class Sensor {
public:
    virtual ~Sensor() = default;
    virtual std::string read() = 0;
    virtual std::string name() = 0;
};

class TempSensor : public Sensor {
public:
    std::string read() override { return "23.5C"; }
    std::string name() override { return "TempSensor"; }
};

class PressureSensor : public Sensor {
public:
    std::string read() override { return "101.3kPa"; }
    std::string name() override { return "PressureSensor"; }
};

// ---------- 简单工厂：v1，只有两个分支，岁月静好 ----------

std::unique_ptr<Sensor> create_sensor_v1(const std::string& type) {
    if (type == "temp")          return std::make_unique<TempSensor>();
    if (type == "pressure")       return std::make_unique<PressureSensor>();
    throw std::invalid_argument("unknown type: " + type);
}

// ---------- 简单工厂：v3，三个版本迭代后 ----------
// v2: 新增 HumiditySensor（伤 A：改了已测试的函数体，全量回归）
// v2.5: TempSensor 构造开始要传校准参数（伤 B：老分支也要动）
// v3: 某并行团队加 GpsSensor，和你的 HumiditySensor 改动撞在同一区域（伤 C）

std::unique_ptr<Sensor> create_sensor_v3(const std::string& type) {
    if (type == "temp") {
        // v2.5 起：校准参数是 TempSensor 的构造知识（第 1 节痛点 3），
        // 泄漏进了工厂函数体。改这个值 = 再改一次已测试代码。
        return std::make_unique<TempSensor>();  // 简化：真实代码要传参
    }
    if (type == "pressure")     return std::make_unique<PressureSensor>();
    if (type == "humidity")     { /* HumiditySensor，团队 A 加的 */ throw std::logic_error("stub"); }
    if (type == "gps")          { /* GpsSensor，团队 B 加的 */      throw std::logic_error("stub"); }
    // v2 新增的"仪式"：每加一个分支，下面的白名单也要同步加，
    // 忘了就是运行时 bug——这就是分支间隐式耦合。
    throw std::invalid_argument("unknown type: " + type);
}

int main() {
    auto s = create_sensor_v1("temp");
    std::printf("v1: %s -> %s\n", s->name().c_str(), s->read().c_str());
    auto p = create_sensor_v1("pressure");
    std::printf("v1: %s -> %s\n", p->name().c_str(), p->read().c_str());
    return 0;
}

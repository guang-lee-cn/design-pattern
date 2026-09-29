// 01-诞生背景：spread.cpp —— 演示"修改扩散"：同一个创建决策出现在 4 个翻译单元
// 编译：g++ -std=c++17 -Wall spread.cpp -o spread && ./spread
// 注：为便于单文件演示，此处用 4 个"扮演不同翻译单元职责"的函数模拟跨文件场景；
//    真实项目里它们分属 4 个 .cpp，由 4 个不同团队维护。

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

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

// ============ 以下 4 个函数模拟 4 个翻译单元（TU）============
// 每个 TU 都有"根据配置创建传感器"的需求，各自写了一份 if-else。
// 共同点：新传感器上线时，4 处全部要改 —— 这就是修改扩散。

// TU1: 初始化模块（业务团队 A 维护）
void init_module(const std::string& type) {
    std::unique_ptr<Sensor> s;
    if (type == "temp")          s = std::make_unique<TempSensor>();
    else if (type == "pressure") s = std::make_unique<PressureSensor>();
    std::printf("[init]    %s -> %s\n", type.c_str(), s->read().c_str());
}

// TU2: 自检模块（业务团队 B 维护）
void selftest_module(const std::string& type) {
    std::unique_ptr<Sensor> s;
    if (type == "temp")          s = std::make_unique<TempSensor>();
    else if (type == "pressure") s = std::make_unique<PressureSensor>();
    std::printf("[selftest] %s ok\n", s->name().c_str());
}

// TU3: 诊断模块（平台团队维护）
void diag_module(const std::string& type) {
    std::unique_ptr<Sensor> s;
    if (type == "temp")          s = std::make_unique<TempSensor>();
    else if (type == "pressure") s = std::make_unique<PressureSensor>();
    std::printf("[diag]    %s raw=%s\n", s->name().c_str(), s->read().c_str());
}

// TU4: 测试桩（QA 维护）
void stub_module(const std::string& type) {
    std::unique_ptr<Sensor> s;
    if (type == "temp")          s = std::make_unique<TempSensor>();
    else if (type == "pressure") s = std::make_unique<PressureSensor>();
    std::printf("[stub]    %s ready\n", s->name().c_str());
}

int main() {
    std::vector<std::string> types = {"temp", "pressure"};
    for (auto& t : types) {
        init_module(t);
        selftest_module(t);
        diag_module(t);
        stub_module(t);
    }
    return 0;
}

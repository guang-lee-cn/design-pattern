// 01-诞生背景：responsibility.cpp —— 拆解痛点 3：责任放错了地方
// 编译：g++ -std=c++17 -Wall responsibility.cpp -o resp && ./resp
//
// 痛点 3 的具体形态：构造知识泄漏。
// "创建哪种传感器"不只是"选个类名"，还包括每个类各自的构造细节
// （参数、默认值、版本差异）。这些知识本属于产品体系，
// 直接 new 的写法却强迫每个调用方都掌握它们。

#include <cstdio>
#include <memory>
#include <string>

// ---------- 产品体系：每个具体类的构造签名各不相同 ----------

class Sensor {
public:
    virtual ~Sensor() = default;
    virtual std::string read() = 0;
};

// TempSensor：需要校准参数，且新旧硬件版本构造方式不同
class TempSensor : public Sensor {
public:
    explicit TempSensor(double calibration_offset) : off_(calibration_offset) {}
    static std::unique_ptr<Sensor> create_v2(int hw_rev) {
        // v2 硬件：校准系数从 OTP 读取，这里简化为固定值
        return std::make_unique<TempSensor>(hw_rev >= 2 ? 0.5 : 0.0);
    }
    std::string read() override { return "23.5C(off=" + std::to_string(off_) + ")"; }
private:
    double off_;
};

// PressureSensor：量程参数决定内部换算，默认值是产品团队的决策
class PressureSensor : public Sensor {
public:
    explicit PressureSensor(double range_kpa = 110.0) : range_(range_kpa) {}
    std::string read() override { return "101.3kPa(range=" + std::to_string(range_) + ")"; }
private:
    double range_;
};

// ---------- 调用方：业务代码被迫掌握产品体系的构造知识 ----------

// 痛点 3 的实体：这三行注释里的知识，凭什么要求写 report() 的人知道？
//   1. TempSensor 构造要传 calibration_offset，不传会编译失败（explicit 无默认值）
//   2. v2 硬件要走 create_v2() 工厂入口（这是 TempSensor 自己的内部决策）
//   3. PressureSensor 有默认量程 110，非标场景才需要改
void report(bool is_v2_hw) {
    std::unique_ptr<Sensor> t;
    if (is_v2_hw) {
        t = TempSensor::create_v2(2);          // <-- 调用方必须懂硬件版本逻辑
    } else {
        t = std::make_unique<TempSensor>(0.0); // <-- 调用方必须懂"0.0 表示不校准"
    }
    auto p = std::make_unique<PressureSensor>(); // <-- 默认量程 110 是产品决策，调用方被动继承

    std::printf("temp:     %s\n", t->read().c_str());
    std::printf("pressure: %s\n", p->read().c_str());
}

int main() {
    report(/*is_v2_hw=*/true);
    return 0;
}

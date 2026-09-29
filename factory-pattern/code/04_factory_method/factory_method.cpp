// 04-工厂方法：factory_method.cpp —— 对照 03_simple_factory/new.cpp
// 编译：g++ -std=c++17 -Wall factory_method.cpp -o fm && ./fm
//
// 核心对照：简单工厂的 create_sensor() 用 if-else 分发（改函数体），
// 工厂方法把"每个产品一个工厂类"（加新类，不改旧代码）。
// 本例演示：新增 HumiditySensor 时，零修改旧文件——只新增两个类。

#include <cstdio>
#include <memory>
#include <string>
#include <vector>

// ---------- 产品体系（与之前相同） ----------

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

// ---------- 工厂方法：抽象工厂 + 每产品一个具体工厂 ----------
// 对照简单工厂的关键差异：
//   简单工厂：create_sensor(type) 一个函数 + if-else 分发
//   工厂方法：SensorFactory 抽象 + TempFactory/PressureFactory 各自实现
// "选哪个产品"的决策从工厂函数体移到了"选哪个工厂对象"的装配点（main）

class SensorFactory {
public:
    virtual ~SensorFactory() = default;
    virtual std::unique_ptr<Sensor> create() = 0;   // <-- 工厂方法本体
    virtual std::string product_name() = 0;
};

class TempFactory : public SensorFactory {
public:
    std::unique_ptr<Sensor> create() override {
        return std::make_unique<TempSensor>();  // 构造知识封在这里
    }
    std::string product_name() override { return "temp"; }
};

class PressureFactory : public SensorFactory {
public:
    std::unique_ptr<Sensor> create() override {
        return std::make_unique<PressureSensor>();
    }
    std::string product_name() override { return "pressure"; }
};

// ---------- 业务代码：只认识抽象，不认识任何具体类 ----------

void report(SensorFactory& factory) {
    auto s = factory.create();
    std::printf("read: %s\n", s->read().c_str());
}

// 装配点：产品目录的登记处。
// 新增 HumiditySensor 时：新增 HumiditySensor + HumidityFactory 两个类，
// 在这里 push_back 一行 —— 三个动作全部是"加"，没有"改"。
// 对照简单工厂：同样要加两个类 + 改 create_sensor() 函数体（含回归）。
std::vector<std::unique_ptr<SensorFactory>> build_catalog() {
    std::vector<std::unique_ptr<SensorFactory>> catalog;
    catalog.push_back(std::make_unique<TempFactory>());
    catalog.push_back(std::make_unique<PressureFactory>());
    return catalog;
}

int main() {
    auto catalog = build_catalog();
    for (auto& f : catalog) {
        std::printf("== %s ==\n", f->product_name().c_str());
        report(*f);
    }
    return 0;
}

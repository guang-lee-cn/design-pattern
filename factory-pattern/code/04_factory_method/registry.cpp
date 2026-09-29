// 04-工厂方法：registry.cpp —— 工厂方法的 C++ 惯用变体：注册表
// 编译：g++ -std=c++17 -Wall registry.cpp -o reg && ./reg
//
// 动机（第 2 节判据 1 的终局）：
//   工厂方法把"改函数体"变成"加新类"，但类数量翻倍（N 产品 = N 工厂类）。
//   注册表用 map<key, 创建函数> 换取同样的 OCP 属性，且类数量回到 1。
//   代价：编译期穷尽性检查换成运行期报错（见文末对照）。

#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

// ---------- 产品体系 ----------

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

// ---------- 注册表式工厂 ----------
// 关键：工厂类本身不包含任何具体产品知识，只有一张表。
// 新增产品 = 调用 register_sensor()，工厂源码一行不改。

class SensorRegistry {
public:
    using Creator = std::function<std::unique_ptr<Sensor>()>;

    // 扩展点：任何模块都可以调它登记自己的传感器
    static void register_sensor(const std::string& type, Creator creator) {
        table()[type] = std::move(creator);
    }

    static std::unique_ptr<Sensor> create(const std::string& type) {
        auto it = table().find(type);
        if (it == table().end()) {
            // 运行期失败——注册表方案的固有代价，见文末对照
            throw std::invalid_argument("unregistered sensor type: " + type);
        }
        return it->second();
    }

    // 供诊断/自检使用：当前已登记的类型清单
    static std::string list() {
        std::string s;
        for (auto& kv : table()) s += kv.first + " ";
        return s;
    }

private:
    static std::map<std::string, Creator>& table() {
        static std::map<std::string, Creator> t;   // 函数内静态，避免初始化顺序问题
        return t;
    }
};

// ---------- 各模块自行登记（可在不同 .cpp 里，天然无合并冲突） ----------
// 真实项目里这两行常放在各自传感器的 .cpp 内，模块加载即完成登记。

void register_builtin_sensors() {
    SensorRegistry::register_sensor("temp", [] { return std::make_unique<TempSensor>(); });
    SensorRegistry::register_sensor("pressure", [] { return std::make_unique<PressureSensor>(); });
}

// ---------- 业务代码 ----------

void report(const std::string& type) {
    auto s = SensorRegistry::create(type);
    std::printf("read: %s\n", s->read().c_str());
}

int main() {
    register_builtin_sensors();
    std::printf("registered: %s\n", SensorRegistry::list().c_str());

    report("temp");
    report("pressure");
    try {
        report("humidity");   // 未登记：运行期报错，且错误信息含类型名
    } catch (const std::exception& e) {
        std::printf("expected error: %s\n", e.what());
    }
    return 0;
}

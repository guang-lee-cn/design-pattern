// =============================================================
// 04_modern_forms.cpp
// C++11 之后，工厂的「多样化」到底多在哪
// 四种形态并列，按「类型何时确定」× 「要不要多态」分类
//
// 编译：g++ -std=c++17 -O2 -Wall 04_modern_forms.cpp
// =============================================================
#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <variant>

// ---------------- 共同的抽象 ----------------
class Sensor {
public:
    virtual ~Sensor() = default;
    virtual int read() const = 0;
    virtual const char* name() const = 0;
};

class TempSensor : public Sensor {
public:
    explicit TempSensor(double cal) : cal_(cal) {}
    int read() const override { return static_cast<int>(cal_ * 10); }
    const char* name() const override { return "TempSensor"; }
private:
    double cal_;
};

class PressureSensor : public Sensor {
public:
    explicit PressureSensor(int range) : range_(range) {}
    int read() const override { return range_; }
    const char* name() const override { return "PressureSensor"; }
private:
    int range_;
};

// =============================================================
// 形态 1：模板工厂 —— 编译期定类型，零虚函数，完美转发
// =============================================================
template <typename T, typename... Args>
std::unique_ptr<T> make_sensor(Args&&... args) {
    return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
}

// 这个模板就是 C++14 标准库 std::make_unique 的全部实现。
// 意义：工厂被「内联」成一个函数模板 —— 不再需要每个产品写一个工厂类。

// =============================================================
// 形态 2：类层次工厂 —— 运行期定类型，GoF 原版
// =============================================================
class SensorFactory {
public:
    virtual ~SensorFactory() = default;
    virtual std::unique_ptr<Sensor> create() const = 0;
};
class TempFactory : public SensorFactory {
public:
    std::unique_ptr<Sensor> create() const override {
        return std::make_unique<TempSensor>(25.0);
    }
};

// =============================================================
// 形态 3：lambda 注册表 —— 运行期定类型，可分散登记
// =============================================================
class SensorRegistry {
public:
    using Creator = std::function<std::unique_ptr<Sensor>()>;
    static void add(const std::string& key, Creator c) {
        table().insert_or_assign(key, std::move(c));
    }
    static std::unique_ptr<Sensor> create(const std::string& key) {
        auto it = table().find(key);              // 必须用 find，不能用 operator[]
        if (it == table().end()) return nullptr;
        return it->second();
    }
private:
    static std::map<std::string, Creator>& table() {
        static std::map<std::string, Creator> t;  // 函数内静态：Meyers 单例
        return t;
    }
};

// =============================================================
// 形态 4：variant —— 编译期定类型集合，值语义，零堆分配
// =============================================================
struct VTemp     { double cal;   int read() const { return static_cast<int>(cal * 10); } };
struct VPressure { int    range; int read() const { return range; } };
using VSensor = std::variant<VTemp, VPressure>;

// 注意：必须用 exhaustive overload，不能用泛型 lambda（第 6 节实测过）
int vread(const VSensor& s) {
    return std::visit([](const auto& x) { return x.read(); }, s);
}

// =============================================================
int main() {
    std::printf("=== 现代 C++ 工厂的四种形态 ===\n\n");

    // 形态 1
    auto s1 = make_sensor<TempSensor>(25.0);
    std::printf("1. 模板工厂   : %s -> %d   (编译期定类型，无虚工厂类)\n",
                s1->name(), s1->read());

    // 形态 2
    TempFactory f2;
    auto s2 = f2.create();
    std::printf("2. 类层次工厂 : %s -> %d   (运行期多态，GoF 原版)\n",
                s2->name(), s2->read());

    // 形态 3
    SensorRegistry::add("temp",     [] { return std::make_unique<TempSensor>(25.0); });
    SensorRegistry::add("pressure", [] { return std::make_unique<PressureSensor>(110); });
    auto s3 = SensorRegistry::create("pressure");
    std::printf("3. lambda 注册表: %s -> %d   (运行期定类型，可分散登记)\n",
                s3->name(), s3->read());

    // 形态 4
    VSensor s4 = VPressure{110};
    std::printf("4. variant     : %d   (sizeof=%zu，无 new、无虚表)\n",
                vread(s4), sizeof(VSensor));

    std::printf("\n--- 四者的分野 ---\n");
    std::printf("  类型何时确定   编译期 -> 形态 1 / 形态 4\n");
    std::printf("                 运行期 -> 形态 2 / 形态 3\n");
    std::printf("  要不要多态     要     -> 形态 2 / 形态 3\n");
    std::printf("                 不要   -> 形态 1 / 形态 4\n");
    return 0;
}

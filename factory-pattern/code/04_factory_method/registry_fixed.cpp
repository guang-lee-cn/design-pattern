// 04b-是不是单例：registry_fixed.cpp
// 编译：g++ -std=c++17 -Wall registry_fixed.cpp -o rf && ./rf
//
// 回答两个问题：
//   Q: SensorRegistry 看起来像单例，但没禁止构造/拷贝/赋值/移动，有问题吗？
//   A: 有三个层面的问题，本例先用"类型特征"把事实量化，再给四种修法。

#include <cstdio>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>

// ---------- 产品体系（略） ----------
class Sensor {
public:
    virtual ~Sensor() = default;
    virtual std::string read() = 0;
};
class TempSensor : public Sensor {
public:
    std::string read() override { return "23.5C"; }
};

using Creator = std::function<std::unique_ptr<Sensor>()>;

// 小工具：把编译期特征打成 "yes/no"
template <class T> const char* YN() { return std::is_copy_constructible<T>::value ? "yes" : "no"; }
template <class T> const char* YNmove() { return std::is_move_constructible<T>::value ? "yes" : "no"; }
template <class T> const char* YNdef() { return std::is_default_constructible<T>::value ? "yes" : "no"; }

// ===========================================================================
// 版本 A：原始写法（只有静态成员，没有特殊成员函数声明）
// ===========================================================================
class RegistryA {
public:
    static void reg(const std::string& k, Creator c) { tbl()[k] = std::move(c); }
    static std::unique_ptr<Sensor> make(const std::string& k) {
        auto it = tbl().find(k);
        if (it == tbl().end()) throw std::runtime_error("unregistered: " + k);
        return it->second();
    }
private:
    static std::map<std::string, Creator>& tbl() {
        static std::map<std::string, Creator> t;
        return t;
    }
};

// ===========================================================================
// 版本 B：显式禁止构造/拷贝/移动 —— 把"不该有实例"写进类型系统
// ===========================================================================
// 【要点】类只要不写任何特殊成员函数，编译器就会隐式生成默认构造、
//        拷贝构造、拷贝赋值、移动构造、移动赋值、析构。
//        "全是静态成员"并不阻止这些函数被生成 —— 你仍可以写 RegistryA a;
//        对象里没有任何数据（sizeof == 1），只是白拿一个毫无意义的实例。
//        显式 delete 之后，这类代码变成编译错误，问题在编译期暴露。
class RegistryB {
public:
    RegistryB() = delete;
    RegistryB(const RegistryB&) = delete;
    RegistryB& operator=(const RegistryB&) = delete;
    RegistryB(RegistryB&&) = delete;
    RegistryB& operator=(RegistryB&&) = delete;

    static void reg(const std::string& k, Creator c) { tbl()[k] = std::move(c); }
    static std::unique_ptr<Sensor> make(const std::string& k) {
        auto it = tbl().find(k);
        if (it == tbl().end()) throw std::runtime_error("unregistered: " + k);
        return it->second();
    }
private:
    static std::map<std::string, Creator>& tbl() {
        static std::map<std::string, Creator> t;
        return t;
    }
};

// ===========================================================================
// 版本 C：命名空间 —— 全静态类在 C++ 里更地道的写法
// ===========================================================================
// 【要点】"全静态成员 + 不可实例化"这一需求，语言本身给的原生答案就是 namespace：
//        天然不可构造、天然不可拷贝，函数写起来还少一层 Registry:: 前缀，
//        而且没有"误以为它是对象"的语义噪音。
namespace sensor_registry {
inline std::map<std::string, Creator>& tbl() {
    static std::map<std::string, Creator> t;
    return t;
}
inline void reg(const std::string& k, Creator c) { tbl()[k] = std::move(c); }
inline std::unique_ptr<Sensor> make(const std::string& k) {
    auto it = tbl().find(k);
    if (it == tbl().end()) throw std::runtime_error("unregistered: " + k);
    return it->second();
}
}  // namespace sensor_registry

// ===========================================================================
// 版本 D：真单例（Meyers Singleton）—— 当注册表需要"可注入/可替换"时才用
// ===========================================================================
// 【要点】版本 A/B/C 的注册表是"进程内唯一"的全局状态，这在测试里很难受：
//        两个测试用例都想注册不同的假传感器，会互相污染。
//        真单例至少把"唯一"这件事显式化，并提供 instance() 作为替换点。
//        注意：单例只解决"唯一性表达"，不解决"全局状态可测性"——
//        要可测还是应该用依赖注入（见文末建议）。
class RegistryD {
public:
    static RegistryD& instance() {
        static RegistryD inst;      // C++11 起线程安全的延迟初始化
        return inst;
    }
    void reg(const std::string& k, Creator c) { tbl_[k] = std::move(c); }
    std::unique_ptr<Sensor> make(const std::string& k) {
        auto it = tbl_.find(k);
        if (it == tbl_.end()) throw std::runtime_error("unregistered: " + k);
        return it->second();
    }
    RegistryD(const RegistryD&) = delete;
    RegistryD& operator=(const RegistryD&) = delete;

private:
    RegistryD() = default;          // 构造私有：只有 instance() 能造
    std::map<std::string, Creator> tbl_;   // 注意：这次表是【成员】而非函数内静态
};

int main() {
    std::printf("=== 类型特征对照（编译期事实）===\n");
    std::printf("%-12s %-10s %-10s %-10s\n", "类型", "默认构造", "拷贝构造", "移动构造");
    std::printf("%-12s %-10s %-10s %-10s\n", "RegistryA",  YNdef<RegistryA>(),  YN<RegistryA>(),  YNmove<RegistryA>());
    std::printf("%-12s %-10s %-10s %-10s\n", "RegistryB",  YNdef<RegistryB>(),  YN<RegistryB>(),  YNmove<RegistryB>());
    std::printf("%-12s %-10s %-10s %-10s\n", "RegistryD",  YNdef<RegistryD>(),  YN<RegistryD>(),  YNmove<RegistryD>());
    std::printf("（RegistryA 全 yes —— 问题不在能否编译，在语义上允许了不该允许的操作）\n\n");

    // 版本 A 的实际危害演示：可以随便造对象、随便拷贝
    // RegistryA a;              // 能编译，对象无意义
    // RegistryA b = a;          // 能编译，拷贝一个空对象
    // 而 RegistryB 下这两行是编译错误 —— 这就是 delete 的价值：
    // 把"设计意图"从注释变成编译器强制。

    std::printf("=== 各版本功能验证 ===\n");
    RegistryB::reg("temp", [] { return std::make_unique<TempSensor>(); });
    sensor_registry::reg("temp", [] { return std::make_unique<TempSensor>(); });
    RegistryD::instance().reg("temp", [] { return std::make_unique<TempSensor>(); });

    std::printf("B: %s\n", RegistryB::make("temp")->read().c_str());
    std::printf("C: %s\n", sensor_registry::make("temp")->read().c_str());
    std::printf("D: %s\n", RegistryD::instance().make("temp")->read().c_str());
    return 0;
}

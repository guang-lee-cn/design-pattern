// 06-代价与边界：variant_alt.cpp —— 封闭集合的现代替代：不工厂，而是 variant
// 编译：g++ -std=c++17 -O2 -Wall variant_alt.cpp -o va && ./va
//
// 什么时候该用 variant 代替工厂：
//   产品集合【封闭且编译期已知】（比如板子上就这 3 种传感器，不会再加）。
//   此时工厂买到的是"运行期可替换"，代价是虚函数/间接调用 + 堆分配；
//   而 variant 买到的是"编译期穷尽性检查 + 零间接调用 + 无堆分配"。
//
// 与工厂的取舍：
//   - 工厂：对"加产品"开放（但要改代码/加类），运行期成本高
//   - variant：加产品必须改源码（编译期强制），但编译器会【逼你处理新分支】

#include <cstdio>
#include <string>
#include <type_traits>
#include <variant>

// ---------------- 三种产品：纯值类型，无虚函数、无基类 ----------------
// 注意：它们不再需要共同基类！这是与工厂方案最大的结构差异。
struct TempSensor {
    int calibration_offset_milli = 0;
    int read() const { return 235 + calibration_offset_milli; }
    const char* name() const { return "TempSensor"; }
};

struct PressureSensor {
    int range_kpa = 110;
    int read() const { return 1013; }
    const char* name() const { return "PressureSensor"; }
};

struct HumiditySensor {
    int rh_percent = 47;
    int read() const { return rh_percent; }
    const char* name() const { return "HumiditySensor"; }
};

// ---------------- 封闭集合：一个 variant 装下全部产品 ----------------
using Sensor = std::variant<TempSensor, PressureSensor, HumiditySensor>;

// 分派写法一：if constexpr（显式、可读，缺点是要手写类型列表）
int read_if_constexpr(const Sensor& s) {
    if (std::holds_alternative<TempSensor>(s)) {
        return std::get<TempSensor>(s).read();
    } else if (std::holds_alternative<PressureSensor>(s)) {
        return std::get<PressureSensor>(s).read();
    } else {
        return std::get<HumiditySensor>(s).read();
    }
}

// 分派写法二：泛型 lambda + visit（推荐，编译器展开为跳转表，无间接调用）
int read_visit(const Sensor& s) {
    return std::visit([](const auto& x) { return x.read(); }, s);
}

// 分派写法三：explicit overload：编译器【强制】你覆盖每一个类型
// —— 加新传感器时，这里会编译报错，而不是运行期炸。这是 variant 最大的价值。
template <class... Ts> struct overloaded : Ts... { using Ts::operator()...; };
template <class... Ts> overloaded(Ts...) -> overloaded<Ts...>;

int read_exhaustive(const Sensor& s) {
    return std::visit(overloaded{
        [](const TempSensor& x)     { return x.read(); },
        [](const PressureSensor& x) { return x.read(); },
        [](const HumiditySensor& x) { return x.read(); },
    }, s);
}

int main() {
    Sensor s1 = TempSensor{10};
    Sensor s2 = PressureSensor{};
    Sensor s3 = HumiditySensor{};

    std::printf("sizeof(TempSensor)      = %zu\n", sizeof(TempSensor));
    std::printf("sizeof(PressureSensor)  = %zu\n", sizeof(PressureSensor));
    std::printf("sizeof(HumiditySensor)  = %zu\n", sizeof(HumiditySensor));
    std::printf("sizeof(Sensor/variant)  = %zu  （= 最大成员 + 判别标签，取整）\n\n",
                sizeof(Sensor));

    std::printf("if constexpr : s1=%d  s2=%d  s3=%d\n",
                read_if_constexpr(s1), read_if_constexpr(s2), read_if_constexpr(s3));
    std::printf("visit(泛型)  : s1=%d  s2=%d  s3=%d\n",
                read_visit(s1), read_visit(s2), read_visit(s3));
    std::printf("visit(穷尽)  : s1=%d  s2=%d  s3=%d\n",
                read_exhaustive(s1), read_exhaustive(s2), read_exhaustive(s3));

    // 无堆分配、无虚表：整个 variant 就是一个栈上的值
    std::printf("\n栈地址：%p  （variant 对象在栈上，没有 new、没有虚表）\n",
                static_cast<const void*>(&s1));

    // 类型清单可以在编译期枚举（工厂方案做不到这一点）
    std::printf("编译期类型数：%zu\n", std::variant_size_v<Sensor>);
    return 0;
}

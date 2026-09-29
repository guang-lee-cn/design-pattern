// 06-代价与边界：bench_dispatch.cpp —— 工厂"多花的那一步"到底值多少纳秒
// 编译：g++ -std=c++17 -O2 -Wall bench_dispatch.cpp -o bench && ./bench
//
// 三个测量纪律（踩过坑才加的）：
//   1. 只测【分派开销】，不测内存分配 —— 全部返回指向静态实例的指针
//   2. 让输入随循环变化（sel = i & 1）—— 否则整段循环会被编译器提到外面，
//      测出 0.00 ns/op 这种假数据
//   3. 累加器用 long long —— 2e7 次累加 1013 会溢出 int（UB 警告）
//
// 五个对照：
//   0. 无工厂：直接拿指针 + 虚调用（基线）
//   1. 简单工厂：string 比较分派
//   2. 工厂方法：虚调用（一次间接跳转）
//   3. 注册表：std::function + map<string,...> 查找
//   4. variant：封闭集合，std::visit 编译期标签分派

#include <chrono>
#include <cstdio>
#include <functional>
#include <map>
#include <string>
#include <variant>

// ---------------- 产品 ----------------
class Sensor {
public:
    virtual ~Sensor() = default;
    virtual int read() const = 0;
};
class TempSensor : public Sensor {
public:
    int read() const override { return 235; }
};
class PressureSensor : public Sensor {
public:
    int read() const override { return 1013; }
};

static TempSensor     g_temp;
static PressureSensor g_press;
static const std::string k_temp  = "temp";
static const std::string k_press = "pressure";

// ---------------- 0. 基线：无工厂 ----------------
inline int run_direct(int sel) {
    const Sensor* p = sel ? static_cast<const Sensor*>(&g_press) : &g_temp;
    return p->read();
}

// ---------------- 1. 简单工厂：string 分派 ----------------
const Sensor* simple_factory(const std::string& type) {
    if (type == "temp")     return &g_temp;
    if (type == "pressure") return &g_press;
    return nullptr;
}
inline int run_simple(int sel) { return simple_factory(sel ? k_press : k_temp)->read(); }

// ---------------- 2. 工厂方法：虚调用 ----------------
class Creator {
public:
    virtual ~Creator() = default;
    virtual const Sensor* create() const = 0;
};
class TempCreator : public Creator {
public:
    const Sensor* create() const override { return &g_temp; }
};
class PressCreator : public Creator {
public:
    const Sensor* create() const override { return &g_press; }
};
static TempCreator  g_temp_creator;
static PressCreator g_press_creator;
inline int run_factory_method(int sel) {
    const Creator* c = sel ? static_cast<const Creator*>(&g_press_creator) : &g_temp_creator;
    return c->create()->read();
}

// ---------------- 3. 注册表：std::function + map ----------------
std::map<std::string, std::function<const Sensor*()>> g_registry = {
    {"temp",     [] { return &g_temp; }},
    {"pressure", [] { return &g_press; }},
};
inline int run_registry(int sel) {
    return g_registry.find(sel ? k_press : k_temp)->second()->read();
}

// ---------------- 4. variant：封闭集合，零间接 ----------------
using AnySensor = std::variant<TempSensor, PressureSensor>;
static const AnySensor g_v1 = TempSensor{};
static const AnySensor g_v2 = PressureSensor{};
inline int run_variant(int sel) {
    return std::visit([](const auto& x) { return x.read(); }, sel ? g_v2 : g_v1);
}

// ---------------- 计时工具 ----------------
template <class F>
double bench(const char* name, long long iters, F&& f) {
    double best = 1e18;
    long long sink = 0;
    for (int round = 0; round < 3; ++round) {
        auto t0 = std::chrono::steady_clock::now();
        long long acc = 0;
        for (long long i = 0; i < iters; ++i) acc += f(static_cast<int>(i & 1));
        auto t1 = std::chrono::steady_clock::now();
        sink += acc;
        double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / iters;
        if (ns < best) best = ns;
    }
    std::printf("%-30s %8.3f ns/op\n", name, best);
    return best;
}

int main() {
    const long long N = 20'000'000;

    std::printf("迭代 %lld 次 ×3 轮取最小值；输入随循环交替（sel = i&1）\n", N);
    std::printf("g++ 13.3 -O2，WSL2/Ubuntu\n\n");
    std::printf("%-30s %s\n", "方案", "耗时");

    double base = bench("0. 无工厂（基线）", N, [](int s) { return run_direct(s); });
    double sf   = bench("1. 简单工厂（string 分派）", N, [](int s) { return run_simple(s); });
    double fm   = bench("2. 工厂方法（虚调用）", N, [](int s) { return run_factory_method(s); });
    double rg   = bench("3. 注册表（map+function）", N, [](int s) { return run_registry(s); });
    double va   = bench("4. variant（visit）", N, [](int s) { return run_variant(s); });

    std::printf("\n相对基线倍数：\n");
    std::printf("  简单工厂        %5.2fx   （多一次 string 逐字符比较）\n", sf / base);
    std::printf("  工厂方法        %5.2fx   （多一次虚表跳转）\n", fm / base);
    std::printf("  注册表          %5.2fx   （多一次 map 查找 + function 间接）\n", rg / base);
    std::printf("  variant         %5.2fx   （编译期分派，几乎零成本）\n", va / base);
    std::printf("\n注：绝对值随 CPU/编译器/标准库变化，看【数量级关系】。\n");
    return 0;
}

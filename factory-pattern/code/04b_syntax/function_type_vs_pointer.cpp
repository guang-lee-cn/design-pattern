// 04b-语法澄清：function_type_vs_pointer.cpp
// 编译：g++ -std=c++17 -Wall function_type_vs_pointer.cpp -o ftp && ./ftp
//
// 回答：std::function<unique_ptr<Sensor>()> 里那个 "unique_ptr<Sensor>()"
//      为什么写成函数类型而不是函数指针类型？包装完会不会变成函数指针？

#include <cstdio>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

class Sensor {
public:
    virtual ~Sensor() = default;
    virtual std::string read() = 0;
};
class TempSensor : public Sensor {
public:
    std::string read() override { return "23.5C"; }
};

// ---------- 两个"长得像"的类型，本质完全不同 ----------
using FuncType    = std::unique_ptr<Sensor>();          // 函数类型
using FuncPtrType = std::unique_ptr<Sensor>(*)();       // 函数指针类型

// 普通函数（用于观察它作为值时怎么"退化"）
std::unique_ptr<Sensor> make_temp() { return std::make_unique<TempSensor>(); }

int main() {
    std::printf("=== 1. 函数类型 vs 函数指针类型：编译器怎么分类 ===\n");
    std::printf("is_function<FuncType>       = %d   (是函数类型)\n", std::is_function<FuncType>::value);
    std::printf("is_pointer <FuncType>       = %d\n", std::is_pointer<FuncType>::value);
    std::printf("is_function<FuncPtrType>    = %d\n", std::is_function<FuncPtrType>::value);
    std::printf("is_pointer <FuncPtrType>    = %d   (是指针类型)\n", std::is_pointer<FuncPtrType>::value);
    std::printf("\n");

    std::printf("=== 2. 关键区别：能不能有变量 ===\n");
    // FuncType  f;   // ❌ 编译错误：不能定义函数类型的变量
    FuncPtrType p = make_temp;   // ✅ 函数指针是一个 8 字节的地址值
    std::printf("sizeof(FuncPtrType)         = %zu  (就是一个指针，8 字节)\n", sizeof(FuncPtrType));
    (void)p;
    std::printf("\n");

    std::printf("=== 3. std::function 用函数类型作模板参数：它自己是个类对象 ===\n");
    using Sig = std::unique_ptr<Sensor>();
    std::function<Sig> fn = make_temp;      // 函数名在这里"退化"成函数指针被装进去
    std::printf("sizeof(std::function<Sig>)  = %zu  (libstdc++ 实测，远大于 8 字节)\n", sizeof(fn));
    std::printf("is_class<std::function<Sig>>= %d   (它是类类型，不是指针)\n",
                std::is_class<std::function<Sig>>::value);
    std::printf("is_pointer<std::function<Sig>> = %d\n", std::is_pointer<std::function<Sig>>::value);
    std::printf("取地址（证明它是一块真正的内存对象）: %p\n", static_cast<void*>(&fn));
    std::printf("\n");

    std::printf("=== 4. 装进去的东西形态各异，但都能用同一个签名调用 ===\n");
    // 4a. 普通函数（传入时退化为函数指针）
    std::function<Sig> a = make_temp;
    // 4b. 无捕获 lambda（也能退化为函数指针）
    std::function<Sig> b = [] { return std::make_unique<TempSensor>(); };
    // 4c. 有捕获 lambda（【不能】退化为函数指针，只能作为对象被装起来）
    int call_count = 0;
    std::function<Sig> c = [&call_count] {
        ++call_count;
        return std::make_unique<TempSensor>();
    };
    // 4d. 仿函数
    struct Maker {
        int n = 0;
        std::unique_ptr<Sensor> operator()() { ++n; return std::make_unique<TempSensor>(); }
    };
    std::function<Sig> d = Maker{};

    std::printf("a(函数):      %s\n", a()->read().c_str());
    std::printf("b(空捕获):    %s\n", b()->read().c_str());
    // 注意：拆成两句。若写成 printf("...", c()->read().c_str(), call_count)，
    // 参数的求值顺序是"不确定顺序"（unspecified order），call_count 可能在
    // c() 执行前就被读走，打印出 0 —— 这是 C++ 里非常常见的自坑。
    auto c_sensor = c();
    std::printf("c(有捕获):    %s   捕获计数=%d\n", c_sensor->read().c_str(), call_count);
    std::printf("d(仿函数):    %s\n", d()->read().c_str());
    std::printf("\n");

    std::printf("=== 5. 无捕获 lambda 可以显式转成函数指针；有捕获的不行 ===\n");
    // 注意：返回类型必须显式写成 unique_ptr<Sensor>。
    // 若让它自动推导成 unique_ptr<TempSensor>，下面的转换会编译失败——
    // 原因：函数指针【不享受返回类型协变】，签名必须逐字匹配（见文末说明）。
    auto no_capture = []() -> std::unique_ptr<Sensor> {
        return std::make_unique<TempSensor>();
    };
    using RawPtr = std::unique_ptr<Sensor>(*)();
    RawPtr raw = +no_capture;    // 一元 + 触发到函数指针的转换
    std::printf("无捕获 lambda -> 函数指针: %s\n", raw()->read().c_str());
    // auto no_capture_ptr = +[&call_count] {...};  // ❌ 编译错误：有捕获不能转
    std::printf("\n");

    std::printf("=== 6. 空 std::function 的真实行为 ===\n");
    std::function<Sig> empty;    // 默认构造 = 空的
    std::printf("empty ? %s\n", empty ? "非空" : "空（operator bool 为 false）");
    try {
        empty();                 // 调用空 function
    } catch (const std::bad_function_call& e) {
        std::printf("调用空 function 抛异常: bad_function_call — %s\n", e.what());
    }
    return 0;
}

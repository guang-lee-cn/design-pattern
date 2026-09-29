// 04a-语法拆解：registry_annotated.cpp
// 编译：g++ -std=c++17 -Wall registry_annotated.cpp -o ra && ./ra
//
// 目的：把 registry.cpp 里每一处"看着眼生"的语法摊开讲。
// 阅读顺序：先看作业务直觉的【大意】，再看【编译器眼里发生了什么】。

#include <cstdio>
#include <functional>   // ① std::function
#include <map>
#include <memory>
#include <stdexcept>
#include <string>

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

// ============================================================================
// ② using Creator = std::function<std::unique_ptr<Sensor>()>;  到底声明了什么
// ============================================================================
// 【大意】Creator 是一个"类型别名"，代表一类可调用对象：
//        无参数，返回 std::unique_ptr<Sensor>。
//
// 【展开】std::function<返回类型(参数类型...)> 是一个"类型擦除容器"：
//        它能把"任何能以该签名调用"的东西装进同一个类型：
//          - 普通函数指针         Sensor* f();
//          - 仿函数（有 operator()）  struct Maker { Sensor* operator()(); };
//          - lambda              [] { return std::make_unique<TempSensor>(); }
//          - 成员函数绑定         std::bind(&X::make, &x)
//        代价：一次间接调用（内部虚函数或函数指针），比直接调 lambda 慢一点点；
//        好处：不同类型的东西能塞进同一个 map 的 value 位置。
//
// 【语法点】为什么能写成 "Sensor()" 而不是 "Sensor (*)()"？
//        std::function 模板参数用的是"函数类型"而不是"函数指针类型"，
//        std::function<void()>  /  std::function<int(double)>，读作"可调用的东西"。
using Creator = std::function<std::unique_ptr<Sensor>()>;

// ============================================================================
// ③ 函数返回引用：static std::map<...>& table()
// ============================================================================
// 【大意】把"那张表"藏在一个函数里，谁想用就调 table() 拿到它的引用。
//
// 【语法点 1】返回类型里的 & —— 返回引用，不是拷贝。
//        没有 & 的话每次调用都会复制整个 map（并且改不到原表）。
//
// 【语法点 2】函数内 static 变量 —— 只在第一次执行到该行时构造，程序结束销毁。
//        - C++11 起：多个线程同时首次调用，编译器保证只有一个线程执行构造，
//          其余线程等待（线程安全的"延迟初始化"，即 Meyers Singleton 同款机制）。
//        - 放在函数里（而非类静态成员）是为了规避"静态初始化顺序问题"：
//          不同 .cpp 的全局变量初始化顺序不确定，函数内静态变量则保证"用时才建"。
//
// 【语法点 3】为什么定义在类内部却写在 .cpp？—— 类内定义的成员函数默认 inline，
//        头文件里写函数内静态变量，多个 .cpp 各自 inline 展开时，
//        标准保证它们指向同一个对象（inline 函数的静态局部变量共享一份）。
std::map<std::string, Creator>& table() {
    static std::map<std::string, Creator> t;   // ③' 全程序唯一的那张表
    return t;
}

// ============================================================================
// ④ register_sensor： table()[type] = std::move(creator);
// ============================================================================
// 【大意】在表里给 type 这个 key 装上"怎么造"的说明；若 key 已存在则覆盖。
//
// 【拆解】逐个成员函数来看这行：
//
//   第 1 步  table()
//            → 拿到 std::map<std::string, Creator> 的引用（左值，可修改）
//
//   第 2 步  table()[type]
//            → 调 std::map::operator[]，它做的事是：
//              a) 若 key 不存在：插入 { "type", Creator{} }
//                 —— 注意 Creator{} 是"空的 std::function"，调用它会抛
//                    std::bad_function_call，但占位合法；
//              b) 返回 value 的【引用】（Creator&）。
//              所以 operator[] 的返回值可以当左值用 —— 这就是能写在 = 左边的原因。
//              另一个副作用：operator[] 会"顺手插入空元素"，
//              查表而不要插入时应该用 find()（registry.cpp 的 create() 就用 find）。
//
//   第 3 步  std::move(creator)
//            → std::move 本身不移动任何东西，它只是一个 static_cast：
//              static_cast<Creator&&>(creator)，把参数当成"右值"看待，
//              于是下一步选中"移动赋值"而不是"拷贝赋值"。
//            → 为什么值得？std::function 内部可能持有 lambda 捕获的状态，
//              移动只需搬指针，拷贝要复制内容（甚至捕获对象不支持拷贝就编译失败）。
//
//   第 4 步  = 赋值
//            → 调 Creator::operator=(Creator&&) 移动赋值，把 creator 的
//              内部状态"搬进"表里的槽位。
//
// 【等价写法】一眼更清楚的两种：
//        t.insert_or_assign(type, std::move(creator));   // C++17，语义最直白
//        t.emplace(type, std::move(creator));            // 已存在则不覆盖
//        t[type] = std::move(creator);                   // 已存在则覆盖（本文档用这个）
void register_sensor(const std::string& type, Creator creator) {
    //                                       ↑ 按值传参：调用者可以 move 进来；
    //                                         内部再 move 进 map，全程零拷贝。
    table()[type] = std::move(creator);
}

// ============================================================================
// ⑤ create：为什么查表用 find 而不是 operator[]
// ============================================================================
// 【大意】查表并调用装好的"创建说明"。
// 【语法点】it->second() —— 调用的不是"某个成员函数名"，而是
//        std::function 的 operator()，也就是"把装进去的东西当作函数调用"。
//        it->second 是 Creator&，后面加 () 就是"调用它"。
//        std::function 的 operator() 内部会转发到真正的 lambda/函数指针。
std::unique_ptr<Sensor> create(const std::string& type) {
    auto it = table().find(type);          // find 不插入，operator[] 会插入
    if (it == table().end()) {
        throw std::invalid_argument("unregistered sensor type: " + type);
    }
    return it->second();                   // <-- 调用装进去的可调用对象
}

// ============================================================================
// ⑥ lambda 到这里才登场：装进表里的到底是什么
// ============================================================================
// 【语法点】[] { return std::make_unique<TempSensor>(); }
//        - []     捕获列表：空表示不捕获外部变量，因此它是"无状态"的，
//                 可以隐式转换成普通函数指针（有捕获就不行）。
//        - ()     参数列表：这里省略了，等价于 ()，与 Creator 签名匹配。
//        - { }    函数体。
//        编译器为每个 lambda 生成一个【唯一的匿名类】，里面定义 operator()。
//        std::function 把这个匿名类对象装进自己内部，之后就能靠统一签名调用。
//
// 【关键理解】注册表方案本质是：把"工厂类层次"降级成了"一堆 lambda 对象"。
//        工厂方法要写 class TempFactory { ... }，注册表只写一个 lambda，
//        类数量从 N 降到 0 —— 代价是没有类型名字、不好在调试器里看。

int main() {
    // 各模块自行登记：这行可以出现在任意 .cpp（包括第三方模块的初始化函数里）
    register_sensor("temp", [] { return std::make_unique<TempSensor>(); });
    register_sensor("pressure", [] { return std::make_unique<TempSensor>(); });  // 演示用，实际换成 PressureSensor

    // 演示 operator[] 的"顺手插入"副作用：问一个不存在的 key，表会变大
    std::printf("before: table size = %zu\n", table().size());
    (void)table()["query_only_should_not_insert"];   // 注意：这行会插入一个空 Creator！
    std::printf("after : table size = %zu  <-- 查表用 [] 的代价\n", table().size());

    auto s = create("temp");
    std::printf("temp -> %s\n", s->read().c_str());
    return 0;
}

// =============================================================
// 02_auto_ptr_trap.cpp
// std::auto_ptr —— 第一次尝试「把所有权写进类型」，方向对，设计错
//
// 同一份代码，三种标准下编译结果完全不同：
//   g++ -std=c++98 -Wall  -> 编译通过，无警告（它是 C++98 的正规设施）
//   g++ -std=c++11 -Wall  -> 编译通过，deprecated 警告
//   g++ -std=c++17        -> 编译失败：std::auto_ptr 已被移除
// =============================================================
#include <cstdio>
#include <memory>

class Product {
public:
    virtual ~Product() { std::printf("    [Product] 析构\n"); }
    virtual const char* name() const = 0;
};

class Sensor : public Product {
public:
    const char* name() const { return "Sensor"; }
};

int main() {
    std::printf("=== std::auto_ptr：看起来像拷贝，实际上是转移 ===\n\n");

    std::auto_ptr<Product> a(new Sensor);
    std::printf("初始状态：\n");
    std::printf("  a.get() = %p\n\n", (void*)a.get());

    std::auto_ptr<Product> b = a;          // ← 这一行「看起来」是拷贝构造
    std::printf("执行 `auto_ptr<Product> b = a;` 之后：\n");
    std::printf("  a.get() = %p    <-- 已经被掏空！\n", (void*)a.get());
    std::printf("  b.get() = %p\n\n", (void*)b.get());

    // a->name();   // 运行期崩溃：a 是空指针，但编译器不拦

    std::printf("结论：auto_ptr 把「拷贝」和「转移所有权」混成了同一个动作。\n");
    std::printf("      任何一次无意的值传递（传参、返回、放容器）都会把对象偷走，\n");
    std::printf("      而且偷走之后源码里看不出来——这才是它被废弃的真正原因。\n");
    return 0;
}

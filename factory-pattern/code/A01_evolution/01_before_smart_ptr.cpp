// =============================================================
// 01_before_smart_ptr.cpp
// 前智能指针时代（C++98）：工厂返回的是「地址」，不是「所有权」
//
// 编译：g++ -std=c++98 -Wall 01_before_smart_ptr.cpp
// =============================================================
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <new>

// ---------------- 产品基类 ----------------
class Product {
public:
    virtual ~Product() {}
    virtual const char* name() const = 0;
    virtual void use() = 0;

    // ⚠️ 这个函数纯粹是为了让演示能跑。真实项目里它不存在——
    //    销毁方式只活在 wiki、口头约定、代码审查，和"上次是谁写的"里。
    virtual const char* release_hint() const = 0;
};

// ---------------- 产品 A：普通 new ----------------
class HeapSensor : public Product {
public:
    static Product* create() { return new HeapSensor(); }
    const char* name() const { return "HeapSensor"; }
    void use() { std::printf("    HeapSensor::use()\n"); }
    const char* release_hint() const { return "delete p"; }
    ~HeapSensor() { std::printf("    [HeapSensor] 析构\n"); }
};

// ---------------- 产品 B：内部 malloc ----------------
class MallocSensor : public Product {
public:
    static Product* create() {
        void* raw = std::malloc(sizeof(MallocSensor));
        return new (raw) MallocSensor();          // placement new
    }
    // 正确的销毁：必须先显式调析构，再 free。
    static void destroy(Product* p) {
        static_cast<MallocSensor*>(p)->~MallocSensor();
        std::free(p);
    }
    const char* name() const { return "MallocSensor"; }
    void use() { std::printf("    MallocSensor::use()\n"); }
    const char* release_hint() const { return "free(p)                 <-- 提示本身就是错的！"; }
    ~MallocSensor() { std::printf("    [MallocSensor] 析构\n"); }
};

// ---------------- 产品 C：来自对象池 ----------------
class PoolSensor : public Product {
public:
    static Product* acquire() { return new PoolSensor(); }
    static void release(Product* p) {
        std::printf("    [PoolSensor] 归还对象池\n");
        delete p;
    }
    const char* name() const { return "PoolSensor"; }
    void use() { std::printf("    PoolSensor::use()\n"); }
    const char* release_hint() const { return "PoolSensor::release(p)  <-- 不是 delete！"; }
    ~PoolSensor() { std::printf("    [PoolSensor] 析构\n"); }
};

// ---------------- 工厂 ----------------
Product* create_product(const char* kind) {
    if (std::strcmp(kind, "heap")   == 0) return HeapSensor::create();
    if (std::strcmp(kind, "malloc") == 0) return MallocSensor::create();
    if (std::strcmp(kind, "pool")   == 0) return PoolSensor::acquire();
    return 0;
}

int main() {
    std::printf("=== C++98 风格工厂：返回值只告诉你「东西在哪」 ===\n\n");

    Product* a = create_product("heap");
    Product* b = create_product("malloc");
    Product* c = create_product("pool");

    std::printf("三个指针的静态类型完全一样：Product*\n");
    std::printf("  a = %p   (heap)\n",  (void*)a);
    std::printf("  b = %p   (malloc)\n",(void*)b);
    std::printf("  c = %p   (pool)\n\n",(void*)c);

    a->use(); b->use(); c->use();

    std::printf("\n--- 释放：三种产品三种销毁方式，逐个查表 ---\n");
    std::printf("  a -> %s\n", a->release_hint());
    std::printf("  b -> %s\n", b->release_hint());
    std::printf("  c -> %s\n", c->release_hint());
    std::printf("\n");

    delete a;                    // 提示写 delete      -> 对
    MallocSensor::destroy(b);    // 提示写 free(p)     -> 提示错了，见下方输出
    PoolSensor::release(c);      // 提示写 release(p)  -> 对

    std::printf("\n程序干净结束。但请注意四件事：\n");
    std::printf("  1. 上面三行写错任意一行，编译器全程不报错。\n");
    std::printf("  2. delete b  -> UB（malloc 的内存配 delete）\n");
    std::printf("  3. delete c  -> 对象池被破坏，下一轮 acquire 拿到脏对象\n");
    std::printf("  4. 最要命的是 b：提示里写的 free(p) 是错的。\n");
    std::printf("     free 不调析构函数 —— 只 free 不析构就是漏掉一半清理。\n");
    std::printf("     正确写法必须先显式 ~MallocSensor() 再 free。\n");
    std::printf("     也就是说：连「提示」本身都写不全，更别说靠人记住了。\n");
    return 0;
}

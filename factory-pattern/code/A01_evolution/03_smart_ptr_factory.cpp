// =============================================================
// 03_smart_ptr_factory.cpp
// C++11 之后：工厂的返回值里能装「所有权 + 销毁策略」
//
// 编译：g++ -std=c++17 -Wall 03_smart_ptr_factory.cpp
// =============================================================
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>
#include <stdexcept>

// ---------------- 分配/释放计数器 ----------------
static int g_alloc = 0;
static int g_free  = 0;

void* operator new(std::size_t n) {
    ++g_alloc;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void* operator new[](std::size_t n) {
    ++g_alloc;
    if (void* p = std::malloc(n ? n : 1)) return p;
    throw std::bad_alloc();
}
void operator delete(void* p) noexcept                { ++g_free; std::free(p); }
void operator delete[](void* p) noexcept              { ++g_free; std::free(p); }
void operator delete(void* p, std::size_t) noexcept   { ++g_free; std::free(p); }
void operator delete[](void* p, std::size_t) noexcept { ++g_free; std::free(p); }

// ---------------- 产品 ----------------
class Product {
public:
    virtual ~Product() = default;
    virtual const char* name() const = 0;
};

class TempSensor : public Product {
public:
    explicit TempSensor(double cal) : cal_(cal) {}
    const char* name() const override { return "TempSensor"; }
private:
    double cal_;
};

// =============================================================
// 1. C++11 起：工厂签名直接把所有权写进去
// =============================================================
std::unique_ptr<Product> make_temp(double cal) {
    return std::unique_ptr<Product>(new TempSensor(cal));
    // C++11 的无奈：make_unique 还不存在（C++14 才进标准）
}

// =============================================================
// 2. 自定义删除器：把「怎么销毁」也装进返回类型
//    —— 这是 C++98 完全做不到的事
// =============================================================
struct FileDeleter {
    void operator()(std::FILE* f) const {
        if (f) {
            std::printf("    [FileDeleter] fclose(%p)\n", (void*)f);
            std::fclose(f);
        }
    }
};
using FilePtr = std::unique_ptr<std::FILE, FileDeleter>;

FilePtr open_writable(const char* path) {
    std::printf("    [open_writable] fopen(\"%s\")\n", path);
    return FilePtr(std::fopen(path, "w"));
}

// =============================================================
// 3. 「所有权窗口」：裸 new 到智能指针接管之间，有一个泄漏缺口
// =============================================================
std::unique_ptr<Product> window_demo(bool boom) {
    Product* raw = new TempSensor(25.0);     // (1) 分配完成
    //        ^^^ 从这里到 (2) 之间，如果抛异常 / 提前 return，
    //            这块内存没人管 —— 这就是那个窗口
    if (boom) {
        throw std::runtime_error("构造中途失败");
    }
    return std::unique_ptr<Product>(raw);    // (2) 所有权才被接管
}

int main() {
    std::printf("=== C++11 之后：所有权进入类型系统 ===\n");
    std::printf("[热身：让 stdout 缓冲区先分配掉，后面的计数才干净]\n\n");

    // ---- 1. unique_ptr 工厂 ----
    auto p = make_temp(25.0);
    std::printf("1. unique_ptr 工厂：%s\n\n", p->name());

    // ---- 2. 自定义删除器 ----
    std::printf("2. 自定义删除器 —— 返回类型自带销毁策略：\n");
    {
        FilePtr fp = open_writable("/tmp/fp_a01_demo.txt");
        if (fp) std::fprintf(fp.get(), "hello\n");
    }   // 离开作用域：FileDeleter::operator() 自动调用 fclose
    std::printf("    已经自动关掉了，调用方不需要知道「该用 fclose 还是 CloseHandle」\n\n");

    // ---- 3. 所有权窗口（用计数器实证泄漏）----
    std::printf("3. 所有权窗口：\n");
    int live_before = g_alloc - g_free;
    try {
        window_demo(true);
    } catch (const std::exception& e) {
        std::printf("    捕获异常：%s\n", e.what());
    }
    int live_after = g_alloc - g_free;
    std::printf("    未释放对象数：%d -> %d   （差 %d = 泄漏的那一个）\n",
                live_before, live_after, live_after - live_before);

    auto ok = window_demo(false);
    std::printf("    正常路径拿到：%s\n\n", ok->name());

    // ---- 4. 分配次数对比 ----
    std::printf("4. 分配次数对比（全局 operator new 计数）：\n");

    g_alloc = g_free = 0;
    { std::shared_ptr<Product> sp(new TempSensor(1.0)); }
    int via_new = g_alloc;

    g_alloc = g_free = 0;
    { std::shared_ptr<Product> sp = std::make_shared<TempSensor>(2.0); }
    int via_make = g_alloc;

    std::printf("    shared_ptr<T>(new T) : %d 次分配\n", via_new);
    std::printf("    make_shared<T>()     : %d 次分配\n", via_make);
    std::printf("    -> make_shared 把控制块和对象合并成一次分配\n");
    std::printf("       代价：对象销毁前那块内存不能回收（weak_ptr 存活时）\n");

    return 0;
}

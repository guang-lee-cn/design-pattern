// 02_singleton_alloc.cpp
// 验证 Fast-DDS DomainParticipantFactory 单例的堆分配代价
//
// 事实来源（Fast-DDS v3.6.2-17, src/cpp/fastdds/domain/DomainParticipantFactory.cpp L87-99）：
//
//   std::shared_ptr<DomainParticipantFactory> DomainParticipantFactory::get_shared_instance()
//   {
//       // Note we need a custom deleter, since the destructor is protected.
//       static std::shared_ptr<DomainParticipantFactory> instance(
//           new DomainParticipantFactory(),
//           [](DomainParticipantFactory* p) { delete p; });
//       return instance;
//   }
//
// 两个问题：
//   Q1  为什么必须写自定义 deleter？（而不是直接 shared_ptr<T> p(new T)）
//   Q2  为什么不用 std::make_shared？（A01 已测过 make_shared 只要 1 次分配）
// 本实验给这两个问题报数。

#include <cstdio>
#include <cstdlib>
#include <memory>
#include <new>

// ---------- 堆分配计数器 ----------
static int g_alloc_count = 0;

void* operator new(std::size_t n)
{
    ++g_alloc_count;
    void* p = std::malloc(n);
    if (nullptr == p)
    {
        throw std::bad_alloc();
    }
    return p;
}

void operator delete(void* p) noexcept
{
    std::free(p);
}

void operator delete(void* p, std::size_t) noexcept
{
    std::free(p);
}

// ---------- 被测对象：复刻 Fast-DDS 的 protected 构造/析构 ----------
struct ProtectedCtor
{
    int payload_[8] = {0};

protected:

    ProtectedCtor() = default;          // ← 与 DomainParticipantFactory() 一致
    ~ProtectedCtor() = default;         // ← 与 virtual ~DomainParticipantFactory() 语义一致

public:

    static ProtectedCtor* create()
    {
        return new ProtectedCtor();
    }

    // 自定义 deleter 必须是「有能力访问 protected 析构」的上下文。
    // 这里做成静态成员：成员函数体内天然拥有访问权限。
    static std::shared_ptr<ProtectedCtor> make_shared_like()
    {
        static std::shared_ptr<ProtectedCtor> instance(
            new ProtectedCtor(),
            [](ProtectedCtor* p)
            {
                delete p;
            });
        return instance;
    }
};

// ---------- 对照组：公开构造/析构 ----------
struct PublicCtor
{
    int payload_[8] = {0};
};

static void print_sep(const char* title)
{
    std::printf("\n========== %s ==========\n", title);
}

int main()
{
    std::printf("Fast-DDS 单例堆分配代价实测\n");

    // ---------- Q2 对照 ----------
    print_sep("分配次数对照");

    g_alloc_count = 0;
    std::shared_ptr<PublicCtor> a = std::make_shared<PublicCtor>();
    int n_make = g_alloc_count;

    g_alloc_count = 0;
    std::shared_ptr<PublicCtor> b(new PublicCtor());
    int n_raw = g_alloc_count;

    std::printf("  make_shared<T>()                  : %d 次分配\n", n_make);
    std::printf("  shared_ptr<T>(new T)              : %d 次分配\n", n_raw);
    std::printf("  差值                              : %d（控制块单独分配）\n", n_raw - n_make);
    std::printf("  说明: make_shared 把对象与控制块放进同一块内存，\n");
    std::printf("        shared_ptr<T>(new T) 必须为控制块再分配一次。\n");

    // ---------- Fast-DDS 的实际选择 ----------
    print_sep("Fast-DDS 的实际选择（protected 构造 + 自定义 deleter）");

    g_alloc_count = 0;
    std::shared_ptr<ProtectedCtor> inst = ProtectedCtor::make_shared_like();
    int n_fastdds = g_alloc_count;
    (void)inst->payload_[0];

    std::printf("  ProtectedCtor::make_shared_like()  : %d 次分配\n", n_fastdds);
    std::printf("  与 make_shared 相比                : +%d 次\n", n_fastdds - n_make);
    std::printf("  这 +%d 次是「构造/析构不可公开访问」的代价。\n", n_fastdds - n_make);

    // ---------- Q1：自定义 deleter 为什么必要 ----------
    print_sep("Q1：自定义 deleter 为什么必要");

    std::printf("  shared_ptr 的默认 deleter 做的是 delete p。\n");
    std::printf("  但 delete 要调用析构函数 -> 需要析构的访问权限。\n");
    std::printf("  析构是 protected 时，标准库内部的 delete 表达式没有访问权限。\n");
    std::printf("  -> 唯一的合法路径：在「有访问权限的上下文」（成员函数内的 lambda）里\n");
    std::printf("     写 delete，把它作为 deleter 交给 shared_ptr。\n");
    std::printf("  Fast-DDS 源码注释原话：\n");
    std::printf("      Note we need a custom deleter, since the destructor is protected.\n");

    // ---------- 魔法静态的线程安全 ----------
    print_sep("附：单例本身为什么不需要锁");

    std::printf("  get_shared_instance() 内部的 static 局部变量初始化受 C++11 保证：\n");
    std::printf("      同一变量的并发初始化只会有一次，其余线程阻塞等待。\n");
    std::printf("  对照 DomainParticipantFactory.cpp：\n");
    std::printf("      L87-99  get_shared_instance()  : 无锁  <- 魔法静态已保证\n");
    std::printf("      L63/108/171/267/281 mtx_participants_ 加锁  <- 保护业务数据\n");
    std::printf("  => 单例的「唯一性」不花锁，但单例管理的「数据」照样要花。\n");

    // ---------- 第二次调用不产生分配 ----------
    print_sep("第二次取实例");

    g_alloc_count = 0;
    std::shared_ptr<ProtectedCtor> again = ProtectedCtor::make_shared_like();
    std::printf("  第二次 make_shared_like()          : %d 次分配\n", g_alloc_count);
    std::printf("  同一实例？                         : %s\n", (again.get() == inst.get()) ? "是" : "否");

    return 0;
}

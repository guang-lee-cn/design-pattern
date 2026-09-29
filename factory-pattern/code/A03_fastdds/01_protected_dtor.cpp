// 01_protected_dtor.cpp
// 验证 Fast-DDS 表达「对象必须走工厂」的手段——protected 析构——的三种后果
//
// 事实来源（Fast-DDS v3.6.2-17）：
//   include/fastdds/dds/rpc/RPCEntity.hpp L54  : ~RPCEntity() = default;   ← protected 非虚
//   include/fastdds/dds/rpc/Service.hpp   L56  : ~Service()   = default;   ← protected 非虚
//   include/fastdds/dds/domain/DomainParticipantFactory.hpp L438 : friend class DomainParticipant;
//                                       L446 : virtual ~DomainParticipantFactory();  ← protected 虚
//   include/fastdds/dds/rpc/impl/rpc_transport_ifaces.hpp L29 :
//                                       virtual ~RequesterTransport() = default;   ← public！
//
// 三个实验：
//   实验 1  protected 非虚析构 + friend 授权 → 派生类析构调用了吗？（UB 实测）
//   实验 2  基类 public 虚析构 + 派生类 protected 析构 → protected 还有防护力吗？
//   实验 3  protected 虚析构 + friend 授权 → 正确写法
//
// 编译时请务必带上 -Wall：编译器自己会对实验 1 发出 UB 警告，这是最直接的证据。

#include <cstdio>

static int g_derived_dtor_calls = 0;   // 哨兵：统计派生类析构函数体被执行几次

struct ParticipantLike;                // 模拟 DomainParticipantImpl（工厂+销毁的持有者）

// ============================================================
// 实验 1：复刻 Fast-DDS RPC 侧写法 —— protected 非虚析构
//         friend 授权方式与 DomainParticipantFactory.hpp L438 同款
// ============================================================
struct NonVirtualBase
{
    virtual const char* name() const = 0;

protected:

    ~NonVirtualBase() = default;      // ← 与 RPCEntity.hpp:54 / Service.hpp:56 一致：非虚

    friend struct ParticipantLike;
};

struct NonVirtualDerived : NonVirtualBase
{
    int* buf_;

    NonVirtualDerived()
        : buf_(new int[4]{1, 2, 3, 4})
    {
    }

    ~NonVirtualDerived()              // 没有 override —— 基类析构不是虚函数，想重写也重写不了
    {
        ++g_derived_dtor_calls;
        delete[] buf_;
    }

    const char* name() const override
    {
        return "NonVirtualDerived";
    }
};

// ============================================================
// 实验 2：基类 public 虚析构 + 派生类 protected 析构
//         —— 对应 rpc_transport_ifaces.hpp 中 RequesterTransport 的情形
// ============================================================
struct PublicDtorBase
{
    virtual ~PublicDtorBase() = default;    // ← public！防护从这里漏掉

    virtual const char* name() const = 0;
};

struct ProtectedDtorDerived : PublicDtorBase
{
    int* buf_;

    ProtectedDtorDerived()
        : buf_(new int[4]{1, 2, 3, 4})
    {
    }

    const char* name() const override
    {
        return "ProtectedDtorDerived";
    }

protected:

    ~ProtectedDtorDerived() override      // 虚函数可以改访问级别，但只在静态类型是本类时生效
    {
        ++g_derived_dtor_calls;
        delete[] buf_;
    }
};

// ============================================================
// 实验 3：正确写法 —— protected 虚析构
// ============================================================
struct VirtualBase
{
    virtual const char* name() const = 0;

protected:

    virtual ~VirtualBase() = default;     // ← 与 DomainParticipantFactory.hpp L446 一致

    friend struct ParticipantLike;
};

struct VirtualDerived : VirtualBase
{
    int* buf_;

    VirtualDerived()
        : buf_(new int[4]{1, 2, 3, 4})
    {
    }

    ~VirtualDerived() override
    {
        ++g_derived_dtor_calls;
        delete[] buf_;
    }

    const char* name() const override
    {
        return "VirtualDerived";
    }
};

// ============================================================
// 模拟 DomainParticipantImpl
//   —— 它既调工厂 create_*，也调 delete_*；靠 friend 取得 protected 析构的访问权
// ============================================================
struct ParticipantLike
{
    static void delete_nonvirtual(NonVirtualBase* p)
    {
        delete p;                      // <- 编译器将在此发出 UB 警告
    }

    static void delete_virtual(VirtualBase* p)
    {
        delete p;
    }
};

static void print_sep(const char* title)
{
    std::printf("\n========== %s ==========\n", title);
}

int main()
{
    std::printf("Fast-DDS「必须走工厂」手段实测（protected 析构的三种后果）\n");

    // ---------- 实验 1 ----------
    print_sep("实验 1：protected 非虚析构 + friend（Fast-DDS RPC 侧写法）");
    g_derived_dtor_calls = 0;
    ParticipantLike::delete_nonvirtual(new NonVirtualDerived());
    std::printf("  派生类析构调用次数        : %d\n", g_derived_dtor_calls);
    std::printf("  期望值                    : 1\n");
    std::printf("  结论                      : %s\n",
            g_derived_dtor_calls == 1 ? "正常" : "派生类析构被跳过 -> buf_ 泄漏 + UB");
    std::printf("  说明: delete p 只调静态类型 NonVirtualBase 的析构函数（非虚），\n");
    std::printf("        NonVirtualDerived::~NonVirtualDerived 不在调用链上。\n");
    std::printf("  旁证: 编译时 -Wall 已给出警告——\n");
    std::printf("        deleting object of abstract class type 'NonVirtualBase' which has\n");
    std::printf("        non-virtual destructor will cause undefined behavior\n");

    // ---------- 实验 2 ----------
    print_sep("实验 2：基类 public 虚析构 + 派生类 protected 析构");
    g_derived_dtor_calls = 0;
    PublicDtorBase* p = new ProtectedDtorDerived();
    delete p;                              // 编译通过！静态类型 PublicDtorBase 的析构是 public
    std::printf("  派生类析构调用次数        : %d\n", g_derived_dtor_calls);
    std::printf("  通过基类指针 delete       : 编译通过（无需 friend，也无警告）\n");
    std::printf("  结论                      : protected 只挡住了「静态类型是本类」的用法\n");
    std::printf("        (a) 栈上定义对象      : 被挡住 ✓\n");
    std::printf("        (b) delete Derived*   : 被挡住 ✓\n");
    std::printf("        (c) delete Base*      : 挡不住 ✗ <- 拿到基类指针的人照样能删\n");

    // ---------- 实验 3 ----------
    print_sep("实验 3：protected 虚析构 + friend（正确写法）");
    g_derived_dtor_calls = 0;
    ParticipantLike::delete_virtual(new VirtualDerived());
    std::printf("  派生类析构调用次数        : %d\n", g_derived_dtor_calls);
    std::printf("  期望值                    : 1\n");
    std::printf("  结论                      : %s\n",
            g_derived_dtor_calls == 1 ? "正常——访问控制与正确析构链同时满足" : "异常");

    // ---------- 汇总 ----------
    print_sep("汇总");
    std::printf("  %-46s %s\n", "写法", "基类指针 delete 的结果");
    std::printf("  %-46s %s\n", "protected 非虚（RPCEntity / Service）", "✗ 派生类析构跳过，UB");
    std::printf("  %-46s %s\n", "public 虚 + 派生 protected（RequesterTransport）", "✓ 析构正确，但防护失效");
    std::printf("  %-46s %s\n", "protected 虚（DomainParticipantFactory）", "✓ 析构正确且防护有效");
    std::printf("\n  两条硬约束同时成立才算「必须走工厂」：\n");
    std::printf("    1) 析构必须是 virtual —— 否则多态删除是 UB\n");
    std::printf("    2) 访问控制必须落在调用方实际持有的静态类型上 —— 否则形同虚设\n");

    return 0;
}

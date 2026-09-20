// ===========================================================================
// §7.2 的实证文件
//
// 把下面三问写成一段既能被编译器检查、也能被运行验证的代码：
//   问一   unique_ptr<ConsoleLogger> 是怎么变成 unique_ptr<Logger> 的？
//   问二   要不要写 std::move？—— 其中"返回函数参数"一项的答案依赖标准版本
//   问三   std::forward 参不参与？
//
// 注：问二的第 ③ 种情形在 C++17 与 C++20 下行为不同，本文件用条件编译
//     把两个版本都钉住。它是全篇唯一一处"答案随标准版本改变"的地方。
//
// 这三问是中文技术博客里最容易写错的地方。多数文章会顺手写成
// "这里发生了移动语义 / 完美转发" —— 两句话都不准确。
// ===========================================================================

#include "fp/logger.h"

#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

namespace {

using fp::ConsoleLogger;
using fp::Logger;

// ---------------------------------------------------------------------------
// 问一：靠 unique_ptr 的**模板转换构造函数**
//       不是移动构造，更不是 dynamic_cast。
//
//     template <class U, class E> unique_ptr(unique_ptr<U, E>&&) noexcept;
//
// 它要求 U* 能隐式转换为 T* —— 即一次编译期的向上转型，零运行时代价。
// ---------------------------------------------------------------------------
std::unique_ptr<Logger> make_by_prvalue() {
    return std::make_unique<ConsoleLogger>();
}

// 编译期就把结论钉死：能不能转、朝哪个方向能转。
static_assert(std::is_constructible<std::unique_ptr<Logger>,
                                    std::unique_ptr<ConsoleLogger>&&>::value,
              "向上转型可以：这正是模板转换构造函数在做的事");
static_assert(!std::is_constructible<std::unique_ptr<ConsoleLogger>,
                                     std::unique_ptr<Logger>&&>::value,
              "向下转型不行：它必须在编译期被挡住");

// ---------------------------------------------------------------------------
// 问二：要不要写 std::move？—— 看返回的是什么，也看标准版本
//
// 这一版刻意先具名、再返回，也就是"return 一个局部对象"。
// ---------------------------------------------------------------------------
std::unique_ptr<Logger> make_from_named_local() {
    auto local = std::make_unique<ConsoleLogger>();
    return local;  // 没写 std::move：C++11 起先按右值做重载决议
}

// 返回**成员变量**在任何标准版本下都必须写 std::move。
// 隐式移动只覆盖"局部自动对象"，以及（C++20 起）函数参数；不覆盖成员。
class Holder {
public:
    void reset() { owned_ = std::make_unique<ConsoleLogger>(); }

    std::unique_ptr<Logger> release_member() {
        return std::move(owned_);  // ← 这里不写就是编译错误
    }

    bool empty() const { return owned_ == nullptr; }

private:
    std::unique_ptr<Logger> owned_;
};

// ---------------------------------------------------------------------------
// 问三：std::forward 参不参与？—— 不参与
//
// 返回语句这一侧根本没有"完美转发"这回事：
//   纯右值      C++17 起直接构造返回对象，连移动构造都没发生（保证的复制省略）
//   局部对象    隐式移动（C++11 起）
//   函数参数    分版本：C++17 不算隐式移动，C++20 起才算
//
// ↓ 下面这个函数同时是问二的第 ③ 种情形和问三的证据，也是全篇唯一
//   "答案随标准版本改变"的地方——所以用条件编译把两个版本都钉住，
//   写错任何一半都编不过。
// ---------------------------------------------------------------------------
std::unique_ptr<Logger> make_from_forwarded_param(std::unique_ptr<Logger>&& incoming) {
#if __cplusplus >= 202002L
    // C++20（P1825R0）起，"指向非 volatile 对象类型的右值引用"被纳入
    // implicitly movable entity，所以这里可以不写。
    return incoming;
#else
    // C++17 及以前，[class.copy.elision] 的措辞是
    // "other than a function or catch-clause parameter"——函数参数被显式排除。
    // 不写 std::move 就会去匹配 unique_ptr 已删除的拷贝构造，直接编译失败。
    return std::move(incoming);
#endif
}

// 对照：真正需要 std::forward 的地方 —— 把实参原样转手给另一个函数。
class Sink {
public:
    void take(std::unique_ptr<Logger> logger) { taken_ = (logger != nullptr); }
    bool taken() const { return taken_; }

private:
    bool taken_ = false;
};

template <typename T>
void forward_to_sink(T&& value, Sink& sink) {
    // 这里才是 std::forward 的正确位置：把 T&& 的左右值属性原样传下去。
    sink.take(std::forward<T>(value));
}

}  // namespace

int main() {
    std::cout << "§7.2  unique_ptr 返回机制：三个问题，全部用代码回答\n";
    std::cout << "====================================================\n\n";

    {
        std::cout << "问一  怎么转型\n";
        auto a = make_by_prvalue();
        auto b = make_from_named_local();
        std::cout << "  返回类型     unique_ptr<Logger>（产品类型在这里被擦除）\n";
        std::cout << "  转型方式     模板转换构造函数 + 编译期向上转型\n";
        std::cout << "  两条 static_assert 已通过：向上可转、向下编译期被拦\n";
        std::cout << "  对象是否有效 a=" << (a ? "是" : "否")
                  << "  b=" << (b ? "是" : "否") << "\n\n";
    }

    {
        std::cout << "问二  要不要写 std::move\n";
        Holder holder;
        holder.reset();
        auto moved = holder.release_member();
        std::cout << "  return local;               局部对象    不写，C++11 起隐式移动\n";
        std::cout << "  return incoming;            函数参数    C++17 必须写；C++20 起可省\n";
        std::cout << "  return std::move(owned_);   成员变量    必须写，所有版本一致\n";
        std::cout << "  本文件实际按 __cplusplus=" << __cplusplus << " 编译（"
                  << (__cplusplus >= 202002L ? "C++20 或更新" : "C++17 及以前") << "）\n";
        std::cout << "  搬走之后 holder 是否已空 → " << (holder.empty() ? "是" : "否")
                  << "（搬出来的对象 " << (moved ? "有效" : "为空") << "）\n\n";
    }

    {
        std::cout << "问三  std::forward 参不参与\n";
        auto via_param = make_from_forwarded_param(std::make_unique<ConsoleLogger>());
        Sink sink;
        forward_to_sink(std::make_unique<ConsoleLogger>(), sink);
        std::cout << "  出现在返回语句里       不参与，写了只是噪音\n";
        std::cout << "  函数参数返回           "
                  << (__cplusplus >= 202002L
                          ? "本次编译走 C++20：不写 std::move 也能过"
                          : "本次编译走 C++17：不写 std::move 就编不过")
                  << "\n";
        std::cout << "  转手传给另一个函数时   必须用，sink.taken="
                  << (sink.taken() ? "true" : "false") << "\n";
        std::cout << "  （若把左值传给 forward_to_sink，会因移动构造被删除而编译失败 ——\n"
                     "    那次失败正是 std::forward 保真传递左右值属性的证明。）\n";
        std::cout << "  经参数返回的对象是否有效 → " << (via_param ? "是" : "否") << "\n\n";
    }

    std::cout << "结论：工厂把 unique_ptr 送出去时，std::forward 一次都不该出现；\n";
    std::cout << "      std::move 只在两处该出现——返回成员变量，以及（仅限 C++17）返回函数参数。\n";
    std::cout << "      在别处看到它们，通常说明作者把「隐式移动」记成了「必须手写移动」。\n";
    return 0;
}

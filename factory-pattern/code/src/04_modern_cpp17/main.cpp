// ===========================================================================
// 第 4 篇主演示：C++17 之后，工厂的几种写法各自长什么样。
//
// 这里刻意把"运行时多态仍是默认"这个结论摆在最后一行 ——
// 上面演示的每一种新写法都有它的适用边界，没有一种能替代虚函数工厂。
// ===========================================================================

#include "expected_compat.h"
#include "fp/logger.h"
#include "modern_factories.h"

#include <iostream>
#include <memory>
#include <string>

namespace {

void demo_template_factory() {
    std::cout << "=== 变化二：模板工厂（§7.3）===\n";
    std::cout << "产品类型由模板参数固定，创建逻辑由非模板基类统一提供。\n\n";

    for (const std::string kind : {"console", "null"}) {
        auto creator = fp::make_creator(kind);
        if (!creator) {
            std::cout << "  " << kind << " -> 没有对应的创建器\n";
            continue;
        }
        auto logger = creator->create();
        std::cout << "  " << kind << " -> 创建成功，静态类型是 fp::Logger（已擦除）\n";
        if (kind == "console") {
            logger->info("  这条来自模板工厂造出来的 ConsoleLogger");
        }
    }

    std::cout << "\n  限制：模板参数只描述「造哪一个」，描述不了「怎么造」。\n"
                 "  所以带构造参数的产品（比如第 1 篇那个 FileLogger）走不了这条路。\n\n";
}

void demo_if_constexpr() {
    std::cout << "=== 变化二（之二）：if constexpr ===\n";

    auto console = fp::create_known_at_compile_time<fp::ConsoleLogger>();
    auto quiet   = fp::create_known_at_compile_time<fp::NullLogger>();

    std::cout << "  编译期已知类型时，未命中的分支根本不参与编译，运行期也没有判断。\n";
    console->info("  这条来自 if constexpr 命中的分支");
    std::cout << "  另一支是否造出对象：" << (quiet ? "是" : "否") << "\n\n";
}

void demo_variant() {
    std::cout << "=== 变化三：封闭产品族用 variant（§7.4）===\n";

    fp::LoggerVariant variant = fp::make_variant_logger(fp::LoggerKind::Console);
    std::cout << "  sizeof(variant<ConsoleLogger, NullLogger>) = "
              << sizeof(fp::LoggerVariant) << " 字节\n";
    std::cout << "  没有 vtable、没有堆分配，访问走 std::visit（编译期展开）\n";
    fp::log_via_variant(variant, "  这条来自 variant 里的 ConsoleLogger");
    std::cout << "\n";
}

void demo_failure_handling() {
    std::cout << "=== 变化五：失败怎么表达（§7.6）===\n";

    const auto ok  = fp::create_logger_optional("console");
    const auto bad = fp::create_logger_optional("nope");

    std::cout << "  optional  \"console\" -> " << (ok.has_value() ? "有值" : "空") << "\n";
    std::cout << "  optional  \"nope\"    -> " << (bad.has_value() ? "有值" : "空")
              << "（知道失败了，但不知道原因）\n";

#if defined(FP_HAS_EXPECTED)
    const auto expected_bad = fp::create_logger_expected("nope");
    std::cout << "  expected  \"nope\"    -> "
              << (expected_bad.has_value() ? "有值"
                                           : fp::to_string(expected_bad.error()))
              << "（连原因一起带回来）\n";
#else
    std::cout << "  未定义 FP_HAS_EXPECTED —— 当前编译器没有 <expected>，"
                 "expected 分支已跳过。\n";
#endif

    std::cout << "\n";
}

}  // namespace

int main() {
    std::cout << "C++17 之后，工厂模式发生了哪些变化\n";
    std::cout << "==================================\n\n";

    demo_template_factory();
    demo_if_constexpr();
    demo_variant();
    demo_failure_handling();

    std::cout << "结论：运行时多态仍是默认选择；\n"
                 "      模板工厂与编译期工厂是局部优化，不是替代品 ——\n"
                 "      它们换掉的是虚函数开销，付出的是「类型不能运行时决定」。\n";
    return 0;
}

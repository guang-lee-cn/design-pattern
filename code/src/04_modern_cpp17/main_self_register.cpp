// ===========================================================================
// 自注册工厂的两个版本共用这一个 main：
//
//   fp_04_self_register_dropped  插件装在 STATIC 库里  → 未被引用 → 被丢弃
//   fp_04_self_register_fixed    插件装在 OBJECT 库里  → 对象必然被纳入
//
// 除了链接方式，两个可执行文件的源码完全相同。
// ===========================================================================

#include "self_register.h"

#include "fp/logger.h"

#include <cstddef>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

// main 自己注册一个 —— 用来证明"注册机制本身是通的"，
// 从而把"注册没生效"这件事精确地归因到链接器身上。
const fp::LoggerRegistrar fp_registrar_console{
    "console",
    []() -> std::unique_ptr<fp::Logger> {
        return std::make_unique<fp::ConsoleLogger>();
    }};

}  // namespace

int main() {
#if defined(FP_04_STATIC_PLUGIN)
    const char* how = "STATIC 库（插件目标文件未被引用）";
#elif defined(FP_04_OBJECT_PLUGIN)
    const char* how = "OBJECT 库（对象文件必然进入链接）";
#else
    const char* how = "unknown";
#endif

    const fp::LoggerRegistry& registry = fp::LoggerRegistry::instance();
    const std::vector<std::string> kinds = registry.kinds();

    std::cout << "插件链接方式：" << how << "\n\n";
    std::cout << "已注册类型 " << kinds.size() << " 个：";
    for (std::size_t i = 0; i < kinds.size(); ++i) {
        std::cout << (i == 0 ? "" : ", ") << kinds[i];
    }
    std::cout << "\n\n";

    std::cout << "plugin-stdout 是否可用："
              << (registry.create("plugin-stdout") ? "是" : "否 —— 它的静态初始化没有执行")
              << "\n\n";

    std::cout << "用 nm 看它到底有没有被链进来：\n"
                 "  nm -C <可执行文件> | grep -i registrar\n"
                 "  （匿名命名空间的符号会在本地符号表里，加 -a 才看得到）\n";
    return 0;
}

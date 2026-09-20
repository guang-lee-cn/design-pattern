#include "fp/logger.h"

#include <iostream>

namespace fp {

void ConsoleLogger::info(const std::string& message) {
    std::cout << "[info]  " << message << '\n';
}

void ConsoleLogger::warn(const std::string& message) {
    std::cout << "[warn]  " << message << '\n';
}

// NullLogger 的两个 override 是类内空实现，这里没有对应的定义。
// 这个文件被做成静态库（而不是 header-only），是为了让第 1 篇能够真实
// 演示「改一个实现要重编译多少翻译单元」——见顶层 CMakeLists.txt 的注释。

}  // namespace fp

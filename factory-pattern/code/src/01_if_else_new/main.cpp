// ===========================================================================
// 这个文件被两个可执行文件**逐字节共用**：
//     fp_01_naive    —— 业务代码拿到的是"直接 new"的实现
//     fp_01_factory  —— 业务代码拿到的是"走工厂"的实现
//
// main 里没有一行代码知道这两种版本的存在。这不是修辞，是 CMake 的事实：
// 两个 target 的源文件列表里，main.cpp 是同一个路径。
// ===========================================================================

#include "order_flow.h"

#include <exception>
#include <iostream>
#include <string>

int main(int argc, char** argv) {
    std::string kind = "console";
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        if (arg == "--kind" && i + 1 < argc) {
            kind = argv[++i];
        }
    }

#if defined(FP_01_NAIVE)
    const char* version = "naive   (业务代码自己 if/else new)";
#elif defined(FP_01_FACTORY)
    const char* version = "factory (业务代码只调 LoggerFactory)";
#else
    const char* version = "unknown";
#endif

    std::cout << "版本：" << version << '\n';
    std::cout << "logger kind：" << kind << "\n\n";

    try {
        fp::run_order_flow(kind);
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }

    std::cout << "\n（两个版本的输出逐字节相同。差别只在 logger 是怎么被造出来的：\n"
                 "  一个散在业务代码里，一个被关进工厂。）\n";
    return 0;
}

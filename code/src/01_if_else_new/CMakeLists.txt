# 第 1 篇《为什么你的 C++ 代码里到处都是 if/else new》配套代码
#
# 两个可执行文件，一份 main.cpp：
#   全部差异只落在 order_flow_naive.cpp / order_flow_factory.cpp 这一个翻译单元里。

set(FP_01_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# 具体日志器。它是"会变化的那个东西"——构造参数一改，谁 include 它谁重编。
# 做成静态库，是为了让"重编译范围有多大"这件事可以被真实观察。
# ---------------------------------------------------------------------------
add_library(fp_01_loggers STATIC
    loggers.cpp)
target_include_directories(fp_01_loggers PUBLIC "${FP_01_INCLUDES}")
target_link_libraries(fp_01_loggers PUBLIC fp_core)

# ---------------------------------------------------------------------------
# 工厂：唯一允许认识具体类的翻译单元。
# 库名带 _lib 后缀，避免和下面同名的可执行文件冲突（CMake 不允许重名 target）。
# ---------------------------------------------------------------------------
add_library(fp_01_factory_lib STATIC
    logger_factory.cpp)
target_include_directories(fp_01_factory_lib PUBLIC "${FP_01_INCLUDES}")
target_link_libraries(fp_01_factory_lib PUBLIC fp_01_loggers)

# ---------------------------------------------------------------------------
# 反例：业务代码自己 if/else new
# ---------------------------------------------------------------------------
add_executable(fp_01_naive
    main.cpp
    order_flow_naive.cpp)
target_include_directories(fp_01_naive PRIVATE "${FP_01_INCLUDES}")
target_link_libraries(fp_01_naive PRIVATE fp_01_loggers)
target_compile_definitions(fp_01_naive PRIVATE FP_01_NAIVE=1)

# ---------------------------------------------------------------------------
# 正例：业务代码只调工厂
#
# 两个可执行文件用的是同一个 main.cpp（同一个路径，不是副本）。
# ---------------------------------------------------------------------------
add_executable(fp_01_factory
    main.cpp
    order_flow_factory.cpp)
target_include_directories(fp_01_factory PRIVATE "${FP_01_INCLUDES}")
target_link_libraries(fp_01_factory PRIVATE fp_01_factory_lib)
target_compile_definitions(fp_01_factory PRIVATE FP_01_FACTORY=1)

# ---------------------------------------------------------------------------
# 想亲手验证"编译依赖变重"这件事，跑这两条命令对比即可：
#
#   g++ -std=c++17 -I src -I include -MM src/01_if_else_new/order_flow_naive.cpp
#       → 依赖列表里出现 loggers.h
#   g++ -std=c++17 -I src -I include -MM src/01_if_else_new/order_flow_factory.cpp
#       → 依赖列表里没有 loggers.h
#
# 也就意味着：改一次 loggers.h 的构造参数，只有前者需要重编。
# ---------------------------------------------------------------------------

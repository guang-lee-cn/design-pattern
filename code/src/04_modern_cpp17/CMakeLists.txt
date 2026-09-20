# 第 4 篇《C++17 之后，工厂模式发生了哪些变化》配套代码

set(FP_04_INCLUDES "${CMAKE_CURRENT_SOURCE_DIR}")

# ---------------------------------------------------------------------------
# 变化二 / 三：模板工厂、if constexpr、封闭产品族 variant
# ---------------------------------------------------------------------------
add_library(fp_04_modern STATIC modern_factories.cpp)
target_include_directories(fp_04_modern PUBLIC "${FP_04_INCLUDES}")
target_link_libraries(fp_04_modern PUBLIC fp_core)

# ---------------------------------------------------------------------------
# 变化五：失败怎么表达。C++17 的 optional 是主线，
# C++23 的 expected 走条件编译，所以本篇在 C++17 编译器上也能全绿。
# ---------------------------------------------------------------------------
add_library(fp_04_expected STATIC expected_compat.cpp)
target_include_directories(fp_04_expected PUBLIC "${FP_04_INCLUDES}")
target_link_libraries(fp_04_expected PUBLIC fp_core)

# ---------------------------------------------------------------------------
# §7.2 三问的实证。单独做成一个可执行文件，因为里面挂了两条 static_assert ——
# 它们不是注释，是编译期就会拦人的断言。
# ---------------------------------------------------------------------------
add_executable(fp_04_probe_unique_ptr probe_unique_ptr.cpp)
target_link_libraries(fp_04_probe_unique_ptr PRIVATE fp_core)
add_test(NAME 04_probe_unique_ptr COMMAND fp_04_probe_unique_ptr)

# ---------------------------------------------------------------------------
# 变化四：自注册工厂。同一个插件翻译单元，装进两种不同的库，
# 用来复现「静态库里的自注册被链接器悄悄抹掉」这个坑。
# ---------------------------------------------------------------------------
add_library(fp_04_registry STATIC self_register.cpp)
target_include_directories(fp_04_registry PUBLIC "${FP_04_INCLUDES}")
target_link_libraries(fp_04_registry PUBLIC fp_core)

# 装进 STATIC 库：插件对外没有任何符号被引用 → 目标文件不会被取出 → 注册不生效。
add_library(fp_04_plugins_static STATIC plugins/plugin_stdout.cpp)
target_include_directories(fp_04_plugins_static PRIVATE "${FP_04_INCLUDES}")
target_link_libraries(fp_04_plugins_static PUBLIC fp_core)

# 装进 OBJECT 库：对象文件必然进入最终链接 → 注册一定生效。
add_library(fp_04_plugins_object OBJECT plugins/plugin_stdout.cpp)
target_include_directories(fp_04_plugins_object PRIVATE "${FP_04_INCLUDES}")
target_link_libraries(fp_04_plugins_object PUBLIC fp_core)

add_executable(fp_04_self_register_dropped main_self_register.cpp)
target_link_libraries(fp_04_self_register_dropped
    PRIVATE fp_04_registry fp_04_plugins_static)
target_compile_definitions(fp_04_self_register_dropped PRIVATE FP_04_STATIC_PLUGIN=1)

add_executable(fp_04_self_register_fixed main_self_register.cpp)
target_link_libraries(fp_04_self_register_fixed
    PRIVATE fp_04_registry fp_04_plugins_object)
target_compile_definitions(fp_04_self_register_fixed PRIVATE FP_04_OBJECT_PLUGIN=1)

# ---------------------------------------------------------------------------
# 另外两条兜底方案，效果与 OBJECT 库相同，但改的是链接方式而不是库类型：
#
#   GNU ld / lld   -Wl,--whole-archive  libplugins.a  -Wl,--no-whole-archive
#   MSVC           /WHOLEARCHIVE:plugins.lib
#   通用（最不推荐，但最好懂）在插件 .cpp 里导出一个函数，由使用者显式调用一次
# ---------------------------------------------------------------------------

# ---------------------------------------------------------------------------
# 总演示
# ---------------------------------------------------------------------------
add_executable(fp_04_main main.cpp)
target_link_libraries(fp_04_main PRIVATE fp_04_modern fp_04_expected)

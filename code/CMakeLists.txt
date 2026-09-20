cmake_minimum_required(VERSION 3.16)

project(factory_pattern_series
        VERSION 1.0.0
        DESCRIPTION "《C++ 工厂模式：从 if/else 到架构边界》系列配套代码"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# ---------------------------------------------------------------------------
# fp_common：所有篇章共享的抽象接口（只放头文件，不做实现）
# ---------------------------------------------------------------------------
add_library(fp_common INTERFACE)
target_include_directories(fp_common INTERFACE
    "${CMAKE_CURRENT_SOURCE_DIR}/include")

if(MSVC)
  target_compile_options(fp_common INTERFACE /W4 /permissive- /Zc:__cplusplus)
else()
  target_compile_options(fp_common INTERFACE
      -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor)
endif()

# ---------------------------------------------------------------------------
# fp_core：共享接口的非内联实现。
# 刻意做成静态库（而不是 header-only）——这样第 1 篇才能真实演示
# “改一个实现要重编译多少”，第 4 篇才能演示“链接器丢弃未引用 .cpp”。
# ---------------------------------------------------------------------------
add_library(fp_core STATIC
    src/fp/logger.cpp)
target_link_libraries(fp_core PUBLIC fp_common)

enable_testing()
add_subdirectory(src)

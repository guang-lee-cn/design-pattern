cmake_minimum_required(VERSION 3.16)

project(factory_pattern
        DESCRIPTION "C++ factory pattern companion code"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Single-config generators (Make/Ninja) honor CMAKE_BUILD_TYPE at configure
# time; default to Release. Switch with: sh run.sh Debug
# (or -DCMAKE_BUILD_TYPE=Debug; Debug outputs go to build-debug/).
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
               Debug Release RelWithDebInfo MinSizeRel)
endif()

if(MSVC)
  add_compile_options(/W4 /permissive- /Zc:__cplusplus)
else()
  add_compile_options(-Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor)
endif()

# Shared headers live in include/ at the project root.
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

message(STATUS "factory_pattern: build type = ${CMAKE_BUILD_TYPE}")

enable_testing()
add_subdirectory(src)

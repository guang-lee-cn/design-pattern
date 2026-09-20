# 每篇正文对应一个子目录。目录未创建时自动跳过，
# 因此本文件不需要在新增篇章时修改。
set(FP_PART_DIRS
    01_if_else_new
    02_three_factories
    03_assembly
    04_modern_cpp17
    05_factory_vs_di
    06_when_not_to_use)

foreach(part IN LISTS FP_PART_DIRS)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${part}/CMakeLists.txt")
    message(STATUS "fp-series: 纳入 ${part}")
    add_subdirectory(${part})
  else()
    message(STATUS "fp-series: 跳过 ${part}（尚未编写）")
  endif()
endforeach()

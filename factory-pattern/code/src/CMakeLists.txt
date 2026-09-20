# 每一节对应一个子目录。目录未创建时自动跳过，
# 因此新增内容时不需要改动本文件，也不需要改动任何已有文件。
set(FP_PART_DIRS
    01_what_is_it
    02_cost
    03_classification
    04_code_compare
    05_scenarios
    06_assembly
    07_language_evolution
    08_boundaries
    09_read_the_sources)

foreach(part IN LISTS FP_PART_DIRS)
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${part}/CMakeLists.txt")
    message(STATUS "factory-pattern: 纳入 ${part}")
    add_subdirectory(${part})
  else()
    message(STATUS "factory-pattern: 跳过 ${part}（尚未编写）")
  endif()
endforeach()

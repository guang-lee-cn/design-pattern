# Auto-discover every scenario program: src/<section>/<scene>/*.cpp.
# Adding a new section or scenario needs NO edit to any CMakeLists.txt;
# just create the directory and drop old.cpp / new.cpp into it.
#
# Target (and executable) names carry the full path:
#   src/02_cost/01_select/old.cpp -> 02_cost_01_select_old
# emitted under build/src/. The section prefix prevents target name clashes
# when different sections reuse the same scenario number.
file(GLOB_RECURSE FP_SOURCES
     CONFIGURE_DEPENDS
     RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}"
     "${CMAKE_CURRENT_SOURCE_DIR}/*/*/*.cpp")

foreach(src IN LISTS FP_SOURCES)
  get_filename_component(scene_dir "${src}" DIRECTORY)        # 02_cost/01_select
  get_filename_component(section   "${scene_dir}" DIRECTORY)  # 02_cost
  get_filename_component(scene     "${scene_dir}" NAME)       # 01_select
  get_filename_component(kind      "${src}" NAME_WE)          # old / new
  set(target "${section}_${scene}_${kind}")
  add_executable("${target}" "${src}")
endforeach()

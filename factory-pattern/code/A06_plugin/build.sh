#!/usr/bin/env bash
# A06 · 运行期插件工厂 —— 一键复现全部实测
#
#   bash build.sh
#
# 五组实验：符号协议 / 可见性 / ABI 双校验 / 卸载陷阱 / 跨 .so 的 RTTI
set -u
CXX=${CXX:-g++}
STD=-std=c++17
HERE=$(cd "$(dirname "$0")" && pwd)
T=/tmp/fp_a06
HIDDEN="-fvisibility=hidden"

rm -rf "$T"; mkdir -p "$T"; cd "$T" || exit 1

"$CXX" $STD -O2 -I"$HERE" "$HERE/host.cpp" -o host -ldl || exit 1

echo "===== 0) 宿主侧布局 ====="
./host sizes

echo
echo "===== 编译插件（-fPIC -shared -fvisibility=hidden）====="
"$CXX" $STD -O2 -fPIC -shared $HIDDEN -I"$HERE" "$HERE/plugin_temp.cpp"        -o libplugin_temp.so
"$CXX" $STD -O2 -fPIC -shared $HIDDEN -I"$HERE" "$HERE/plugin_pressure.cpp"    -o libplugin_pressure.so
"$CXX" $STD -O2 -fPIC -shared $HIDDEN -I"$HERE" "$HERE/plugin_mangled.cpp"     -o libplugin_mangled.so
"$CXX" $STD -O2 -fPIC -shared $HIDDEN -I"$HERE" "$HERE/plugin_abi_drifted.cpp" -o libabi_drifted.so
"$CXX" $STD -O2 -fPIC -shared $HIDDEN -I"$HERE" -DABI_BUMP "$HERE/plugin_abi_drifted.cpp" -o libabi_bumped.so
"$CXX" $STD -O2 -fPIC -shared -I"$HERE" "$HERE/plugin_temp.cpp" -o libplugin_temp_vis.so
echo "OK（6 个 .so）"

echo
echo "===== 实验 1 · 符号协议：dlsym 按名字找，C++ 修饰后的名字找不着 ====="
echo "--- nm -D --defined-only libplugin_temp.so ---"
nm -D --defined-only libplugin_temp.so | head -20
echo "--- nm -D --defined-only libplugin_mangled.so ---"
nm -D --defined-only libplugin_mangled.so | head -20
echo
echo "--- 宿主找 \"plugin_entry\" ---"
./host load ./libplugin_temp.so;    echo "退出码=$?"
./host load ./libplugin_mangled.so; echo "退出码=$?"

echo
echo "===== 实验 2 · 可见性：-fvisibility=hidden 到底省掉了多少导出符号 ====="
"$CXX" $STD -O0 -fPIC -shared $HIDDEN -I"$HERE" "$HERE/plugin_temp.cpp" -o libplugin_temp_hidden_O0.so
"$CXX" $STD -O0 -fPIC -shared          -I"$HERE" "$HERE/plugin_temp.cpp" -o libplugin_temp_vis_O0.so
printf "%-22s %s\n" "hidden    -O2:" "$(nm -D --defined-only libplugin_temp.so        | wc -l)"
printf "%-22s %s\n" "默认可见  -O2:" "$(nm -D --defined-only libplugin_temp_vis.so    | wc -l)"
printf "%-22s %s\n" "hidden    -O0:" "$(nm -D --defined-only libplugin_temp_hidden_O0.so | wc -l)"
printf "%-22s %s\n" "默认可见  -O0:" "$(nm -D --defined-only libplugin_temp_vis_O0.so  | wc -l)"
echo "--- 默认可见性 -O0 构建到底导出了什么 ---"
nm -D --defined-only libplugin_temp_vis_O0.so

echo
echo "===== 实验 3 · ABI 双校验 ====="
echo "--- 3a 布局漂移 + 版本号忘了改，宿主双校验（推荐路径）---"
./host load ./libabi_drifted.so;      echo "退出码=$?"
echo "--- 3b 同样输入，宿主只校验 version（危险路径）---"
./host version-only ./libabi_drifted.so; echo "退出码=$?"
echo "--- 3c 布局漂移 + 版本号已改（纪律正确）---"
./host load ./libabi_bumped.so;       echo "退出码=$?"

echo
echo "===== 实验 4 · 卸载陷阱 ====="
echo "--- 4a dlclose 之后继续用 ---"
./host unload-use ./libplugin_temp.so;      echo "退出码=$?"
echo "--- 4b 改用 RTLD_NODELETE ---"
./host unload-nodelete ./libplugin_temp.so; echo "退出码=$?"

echo
echo "===== 实验 5 · 跨 .so 的 RTTI ====="
"$CXX" $STD -O2 -fPIC -shared          -I"$HERE" "$HERE/rtti_plugin.cpp" -o librtti_vis.so
"$CXX" $STD -O2 -fPIC -shared $HIDDEN  -I"$HERE" "$HERE/rtti_plugin.cpp" -o librtti_hidden.so
"$CXX" $STD -O2 -I"$HERE" "$HERE/rtti_host.cpp" -o rtti_host -ldl
echo "--- 5a 插件默认可见性 ---"
./rtti_host ./librtti_vis.so;    echo "退出码=$?"
echo "--- 5b 插件 -fvisibility=hidden ---"
./rtti_host ./librtti_hidden.so; echo "退出码=$?"
echo "--- 5c 卸载后再碰对象（typeid / 虚调用）---"
./rtti_host ./librtti_vis.so unload; echo "退出码=$?"

echo
echo "全部完成。"

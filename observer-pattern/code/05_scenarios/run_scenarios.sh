#!/bin/sh
# 依次构建并运行第 5 节的三个场景探针。
#
#   场景 1  GUI 事件      —— Qt 5 signal/slot，Direct / Queued / Auto 对照（真编译真跑）
#   场景 2  配置热更新    —— 内核通知链，内核态代码跑不了，就地摘引头文件原文
#   场景 3  文件/缓存变更 —— inotify，跨进程边界（真编译真跑）
#
# 依赖：g++（C++17）、Qt5Core 开发文件、Linux（inotify）。
# moc 不在 PATH 里时，用 MOC=/path/to/moc ./run_scenarios.sh 指定。

set -e
cd "$(dirname "$0")"

BUILD=${TMPDIR:-/tmp}/observer05-build
WATCH=/tmp/observer05-watch
MOC=${MOC:-moc}

mkdir -p "$BUILD" "$WATCH"

bar() {
  echo
  echo "############################################################"
  echo "# $1"
  echo "############################################################"
}

bar "场景 1/3  GUI 事件（Qt）"
pkg-config --exists Qt5Core || { echo "找不到 Qt5Core 开发文件（装 qtbase5-dev）"; exit 1; }
echo "Qt: $(pkg-config --modversion Qt5Core)   moc: $(command -v "$MOC")"
"$MOC" qt_connection_types.cpp -o "$BUILD/qt_connection_types.moc"
g++ -std=c++17 -Wall -Wextra -fPIC \
    -I"$BUILD" $(pkg-config --cflags Qt5Core) \
    qt_connection_types.cpp \
    $(pkg-config --libs Qt5Core) \
    -o "$BUILD/qt_probe"
"$BUILD/qt_probe"

bar "场景 2/3  配置热更新（内核通知链）"
NOTIFIER=$(ls /usr/src/*/include/linux/notifier.h 2>/dev/null | head -1)
if [ -n "$NOTIFIER" ]; then
  echo "内核态代码，用户空间跑不了。就地摘引：$NOTIFIER"
  echo
  echo "--- 动机（头文件开头，Alan Cox）---"
  grep -n "instead of hard coded call lists" "$NOTIFIER" || true
  echo "--- 名单放在哪 ---"
  grep -n "^struct notifier_block\|struct .*notifier_head {" "$NOTIFIER" || true
  echo "--- 回调的返回值能中止链 ---"
  grep -n "NOTIFY_STOP_MASK\|NOTIFY_BAD\|NOTIFY_DONE" "$NOTIFIER" || true
  echo "--- 链内不许注销 ---"
  grep -n "must not_ be called from within" "$NOTIFIER" || true
else
  echo "本机没有内核头文件，跳过（正文里给了引用行号）"
fi

bar "场景 3/3  文件/缓存变更（inotify，跨进程）"
g++ -std=c++17 -Wall -Wextra \
    inotify_cross_process.cpp \
    -o "$BUILD/inotify_probe"
"$BUILD/inotify_probe"

echo
echo "构建产物在 $BUILD ，监控目录 $WATCH"

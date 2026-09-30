#!/bin/sh
# 第 9 节：四个真实项目案例的回源取证
#
# 一次跑完四件事：
#   案例 1  Qt 5.15 signals/slots        —— 真编译真跑（moc + g++），连接名单的六问
#   案例 2  Boost.Signals2 1.83          —— 真编译真跑（header-only），弱引用轨迹的八问
#   案例 3  Linux 6.8 notifier_chain     —— 内核态代码跑不了，就地摘引头文件原文
#   案例 4  Fast-DDS 3.6.2 DataReaderListener —— 就地摘引源码
#
# 案例 3 / 4 的源码根由环境变量指定，缺省时只跳过该案例，不影响前两个：
#   KERNEL_HEADER=/usr/src/linux-headers-$(uname -r)/include/linux/notifier.h
#   FASTDDS_ROOT=/path/to/fast-dds
#
# 案例 1 需要 qtbase5-dev（pkg-config Qt5Core）与 moc；moc 不在 PATH 时用 MOC= 指定。
# 脚本只取证，不下结论 —— 结论写在第 9 节正文里。产物落在临时目录，不污染仓库。

cd "$(dirname "$0")"

BUILD=${TMPDIR:-/tmp}/observer09-build
mkdir -p "$BUILD"

bar() {
  echo
  echo "############################################################"
  echo "# $1"
  echo "############################################################"
}

bar "案例 1/4  Qt 5.15 signals/slots（真编译真跑）"
if pkg-config --exists Qt5Core; then
  MOC=${MOC:-moc}
  echo "Qt: $(pkg-config --modversion Qt5Core)   moc: $(command -v "$MOC")"
  "$MOC" qt_connection_lifetime.cpp -o "$BUILD/qt_connection_lifetime.moc"
  g++ -std=c++17 -Wall -Wextra -fPIC -I"$BUILD" $(pkg-config --cflags Qt5Core) \
      qt_connection_lifetime.cpp $(pkg-config --libs Qt5Core) -o "$BUILD/qt_probe"
  "$BUILD/qt_probe"
else
  echo "找不到 Qt5Core 开发文件（装 qtbase5-dev），跳过"
fi

bar "案例 2/4  Boost.Signals2（真编译真跑）"
if [ -f /usr/include/boost/signals2/signal.hpp ]; then
  echo "Boost: $(grep -m1 '#define BOOST_LIB_VERSION' /usr/include/boost/version.hpp | cut -d'"' -f2)"
  g++ -std=c++17 -Wall -Wextra signals2_lifetime.cpp -o "$BUILD/s2_probe"
  "$BUILD/s2_probe"
else
  echo "找不到 Boost.Signals2 头文件，跳过"
fi

bar "案例 3/4  Linux notifier_chain（就地摘引）"
KH=${KERNEL_HEADER:-$(ls /usr/src/*/include/linux/notifier.h 2>/dev/null | head -1)}
if [ -n "$KH" ] && [ -f "$KH" ]; then
  echo "头文件：$KH  （$(wc -l < "$KH") 行）"
  echo
  echo "--- 名单放在哪：侵入式单链表 + 优先级 ---"
  grep -n -A5 '^struct notifier_block {' "$KH"
  echo
  echo "--- 四种链，锁策略各自不同（raw 完全不锁）---"
  grep -n -A4 '^struct raw_notifier_head {\|^struct blocking_notifier_head {' "$KH"
  echo
  echo "--- 回调返回值：不只是「停不停」，还是 errno 通道 ---"
  grep -n 'define NOTIFY_STOP_MASK\|define NOTIFY_STOP\b' "$KH"
  grep -n -A4 'static inline int notifier_from_errno' "$KH"
  echo
  echo "--- 生命周期：链内禁止注销；同优先级注册有单独入口 ---"
  grep -n 'must not_ be called from within' "$KH"
  grep -n 'register_unique_prio' "$KH"
  echo
  echo "--- 可数性：链头只回答「空不空」---"
  grep -n 'call_chain_is_empty' "$KH"
else
  echo "未找到内核头文件（用 KERNEL_HEADER= 指定），跳过"
fi

bar "案例 4/4  Fast-DDS DataReaderListener（就地摘引）"
FD=${FASTDDS_ROOT:-}
if [ -n "$FD" ] && [ -f "$FD/src/cpp/fastdds/subscriber/DataReaderImpl.cpp" ]; then
  echo "源码根：$FD"
  echo
  echo "--- 监听器是单个指针，不是名单 ---"
  grep -n 'DataReaderListener\* listener_\|mutable std::mutex listener_mutex_' \
      "$FD/src/cpp/fastdds/subscriber/DataReaderImpl.hpp"
  echo
  echo "--- 注册 = 一次赋值；注销 = set_listener(nullptr) ---"
  sed -n '1413,1425p' "$FD/src/cpp/fastdds/subscriber/DataReaderImpl.cpp"
  echo
  echo "--- 投递前的两级判定：本实体的 listener + 状态掩码，否则向上一级冒泡 ---"
  sed -n '1901,1915p' "$FD/src/cpp/fastdds/subscriber/DataReaderImpl.cpp"
  echo
  echo "--- 冒泡第二级：Subscriber 再问 Participant ---"
  sed -n '708,717p' "$FD/src/cpp/fastdds/subscriber/SubscriberImpl.cpp"
  echo
  echo "--- 生命周期：析构前把整条链的 listener 摘空 ---"
  grep -rn 'quiet destruction' "$FD/src/cpp/fastdds" "$FD/include" | sed "s#$FD/##"
  grep -n 'set_listener(nullptr)' "$FD/src/cpp/fastdds/subscriber/DataReaderImpl.cpp"
  echo
  echo "--- 数据到达时的口径：on_data_on_readers 优先于 on_data_available ---"
  sed -n '394,409p' "$FD/src/cpp/fastdds/subscriber/DataReaderImpl.cpp"
else
  echo "未指定 Fast-DDS 源码根（用 FASTDDS_ROOT= 指定），跳过"
fi

echo
echo "构建产物在 $BUILD"

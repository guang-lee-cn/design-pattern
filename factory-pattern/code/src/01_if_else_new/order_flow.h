#pragma once

#include <string>

namespace fp {

/// 一段业务代码。
///
/// 它需要日志器，于是它得先把日志器搞到手 —— **怎么拿到，就是本篇的全部主题**。
///
/// 这个声明被两个版本共用，里面没有任何版本痕迹：调用方不需要知道
/// 日志器是直接 new 出来的，还是从工厂拿的。这正是本篇想证明的那句话 ——
/// 「创建方式的变化，不该穿透到使用方」。
void run_order_flow(const std::string& logger_kind);

}  // namespace fp

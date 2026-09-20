#pragma once

#include "fp/logger.h"

#include <memory>
#include <string>
#include <vector>

namespace fp {

/// 简单工厂：把"创建哪个具体类"从业务代码里挪到这一个类里。
///
/// 它是整个系列的起点。后面几篇会对它做四次改动，每次解决一个问题、
/// 也各付一次代价：
///   第 2 篇 —— 回答"它算不算 GoF 模式"（它不算，它是 idiom）
///   第 3 篇 —— 换成注册表，消掉这里的 if/else
///   第 4 篇 —— 返回类型从 nullptr 换成 optional / expected
///   第 5 篇 —— 把它限制在组合根里，不再被业务代码调用
class LoggerFactory {
public:
    /// 返回 nullptr 表示没有这种类型。
    ///
    /// 用 nullptr 而不是异常是刻意的：第 4 篇会专门讨论「失败怎么表达」
    /// 这件事，从 nullptr 到 optional 再到 expected。
    static std::unique_ptr<Logger> create(const std::string& kind);

    /// 当前支持的类型名。
    ///
    /// 它必须手工维护 —— 新增一个具体类，就要记得回来改这里。
    /// 第 4 篇的自注册工厂会让这个函数不再需要存在。
    static std::vector<std::string> supported_kinds();
};

}  // namespace fp

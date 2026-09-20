#pragma once

#include <string>

namespace fp {

/// 日志抽象。
///
/// 本系列各篇共用。它本身不是文章主题——在这里的作用是充当
/// “除支付处理器之外的第二个被注入的依赖”，用来演示：
/// 一个对象的依赖可能不止一个，而它们的所有权策略可以不同。
class Logger {
public:
    virtual ~Logger() = default;

    virtual void info(const std::string& message) = 0;
    virtual void warn(const std::string& message) = 0;
};

/// 打印到 stdout 的实现，供各篇 demo 使用。
class ConsoleLogger final : public Logger {
public:
    void info(const std::string& message) override;
    void warn(const std::string& message) override;
};

/// 什么都不做的实现。
///
/// 它存在的理由很实际：第 1 篇要用它做「同一份 main、同一份输出」的对照中
/// 那个**完全静默**的一侧；第 4 篇要用它做自注册工厂里「不需要构造参数」的
/// 那个产品。放在共享头里，是为了避免每篇各写一份同名的空实现。
class NullLogger final : public Logger {
public:
    void info(const std::string& /*message*/) override {}
    void warn(const std::string& /*message*/) override {}
};

}  // namespace fp

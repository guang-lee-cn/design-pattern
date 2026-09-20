#pragma once

#include "fp/logger.h"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace fp {

using LoggerCreatorFn = std::function<std::unique_ptr<Logger>()>;

/// 注册表：把「类型名 → 创建函数」的映射集中到一个进程内唯一的地方。
///
/// 第 3 篇会给它加上产品族约束；本篇先只做"注册"这一件事，
/// 目的是消掉工厂里那个必须手工维护的 if/else 链。
class LoggerRegistry {
public:
    static LoggerRegistry& instance();

    /// 重复注册返回 false，并且**不覆盖**已有项。
    /// 静默覆盖是调试噩梦：两个模块都注册同一个名字时，你应该立刻知道。
    bool add(std::string_view kind, LoggerCreatorFn creator);

    /// 未注册时返回 nullptr。
    std::unique_ptr<Logger> create(std::string_view kind) const;

    /// 已注册的类型名（已排序），供 demo 打印。
    std::vector<std::string> kinds() const;

private:
    LoggerRegistry() = default;

    std::unordered_map<std::string, LoggerCreatorFn> creators_;
};

/// 自注册句柄。
///
/// 在任意一个 .cpp 里定义一个文件级静态实例，注册就完成了。
///
/// 注意 instance() 用的是**函数内的静态局部变量**（C++11 起初始化线程安全），
/// 所以本句柄的静态初始化顺序与注册表自身的构造顺序无关 ——
/// 这是踩坑表里"静态初始化顺序"那一条的规避方式，不是巧合。
class LoggerRegistrar {
public:
    LoggerRegistrar(std::string_view kind, LoggerCreatorFn creator) {
        LoggerRegistry::instance().add(kind, std::move(creator));
    }
};

}  // namespace fp

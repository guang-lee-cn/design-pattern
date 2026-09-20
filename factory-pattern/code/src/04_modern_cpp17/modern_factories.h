#pragma once

#include "fp/logger.h"

#include <memory>
#include <string>
#include <string_view>
#include <type_traits>
#include <variant>

namespace fp {

/// 依赖某个模板参数的断言辅助。
/// 直接写 static_assert(false) 会在模板定义处就报错；
/// 加一层 T 之后，只有真正实例化到这一支时才触发。
template <typename>
inline constexpr bool always_false_v = false;

// ---------------------------------------------------------------------------
// 变化二：模板工厂（§7.3）
//
// 模板参数只描述「造哪一个」，描述不了「怎么造」。
// 两个 static_assert 把这个限制钉死在编译期 —— 不是靠文档提醒，是靠编译器拦住。
// ---------------------------------------------------------------------------
class LoggerCreatorBase {
public:
    virtual ~LoggerCreatorBase() = default;
    virtual std::unique_ptr<Logger> create() const = 0;
};

template <typename T>
class LoggerCreator final : public LoggerCreatorBase {
    static_assert(std::is_base_of<Logger, T>::value,
                  "LoggerCreator<T> 要求 T 派生自 fp::Logger");
    static_assert(std::is_default_constructible<T>::value,
                  "模板工厂要求产品可以默认构造 —— 这就是它的根本限制");

public:
    std::unique_ptr<Logger> create() const override {
        return std::make_unique<T>();
    }
};

/// 按类型名挑一个创建器。返回 nullptr 表示没有这个类型。
std::unique_ptr<LoggerCreatorBase> make_creator(std::string_view kind);

// ---------------------------------------------------------------------------
// 变化二的另一支：if constexpr
//
// 编译期就知道类型时，没被选中的分支根本不参与编译，运行期也没有判断。
// 代价是：类型必须在编译期确定，读不了配置文件。
// ---------------------------------------------------------------------------
template <typename T>
std::unique_ptr<Logger> create_known_at_compile_time() {
    if constexpr (std::is_same<T, ConsoleLogger>::value) {
        return std::make_unique<ConsoleLogger>();
    } else if constexpr (std::is_same<T, NullLogger>::value) {
        return std::make_unique<NullLogger>();
    } else {
        static_assert(always_false_v<T>, "未知的 Logger 类型 —— 请在这里补一个分支");
        return nullptr;  // 只有非法类型才可能走到；那时 static_assert 已经先报错了
    }
}

// ---------------------------------------------------------------------------
// 变化三：封闭产品族用 variant（§7.4）
//
// 产品族在编译期已知且封闭时，可以不要 vtable、不要堆分配，走值语义。
// 代价同样是开放性：新增一个产品就要改下面这个 using。
// ---------------------------------------------------------------------------
enum class LoggerKind { Console, Null };

using LoggerVariant = std::variant<ConsoleLogger, NullLogger>;

LoggerVariant make_variant_logger(LoggerKind kind);

/// 用 std::visit 访问：编译期展开成重载集合，没有虚调用。
void log_via_variant(LoggerVariant& variant, const std::string& message);

}  // namespace fp

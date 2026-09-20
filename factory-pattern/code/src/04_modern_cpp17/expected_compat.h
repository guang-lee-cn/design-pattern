#pragma once

// ---------------------------------------------------------------------------
// 变化五：失败怎么表达（§7.6）
//
//   C++11/14   nullptr 或异常
//   C++17      std::optional
//   C++23      std::expected
//
// 本仓库主代码保持 C++17 兼容，所以 expected 分支走条件编译；
// 同时给出 C++17 的降级写法 —— 读者不需要 C++23 编译器也能跑通全部例子。
// ---------------------------------------------------------------------------

#include "fp/logger.h"

#include <memory>
#include <optional>
#include <string>
#include <string_view>

#if defined(__has_include)
#  if __has_include(<expected>)
#    include <expected>
#    if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L
#      define FP_HAS_EXPECTED 1
#    endif
#  endif
#endif

namespace fp {

enum class CreateError { UnknownType, BadConfig };

inline const char* to_string(CreateError error) {
    switch (error) {
        case CreateError::UnknownType: return "unknown-type";
        case CreateError::BadConfig:   return "bad-config";
    }
    return "unknown-error";
}

/// C++17 的写法。
///
/// 它能表达"失败了"，但表达不了"为什么失败" ——
/// 而且一旦产品是 move-only，optional<unique_ptr<T>> 这层嵌套也显得笨重。
std::optional<std::unique_ptr<Logger>> create_logger_optional(std::string_view kind);

#if defined(FP_HAS_EXPECTED)
/// C++23 的写法：失败原因随返回值一起带出来。
/// 不需要异常，也不需要出参 —— 这是它相对 optional 的全部收益。
std::expected<std::unique_ptr<Logger>, CreateError> create_logger_expected(std::string_view kind);
#endif

}  // namespace fp

#include "expected_compat.h"

namespace fp {

std::optional<std::unique_ptr<Logger>> create_logger_optional(std::string_view kind) {
    if (kind == "console") {
        return std::make_unique<ConsoleLogger>();
    }
    if (kind == "null") {
        return std::make_unique<NullLogger>();
    }
    return std::nullopt;
}

#if defined(FP_HAS_EXPECTED)
std::expected<std::unique_ptr<Logger>, CreateError> create_logger_expected(std::string_view kind) {
    if (kind == "console") {
        return std::make_unique<ConsoleLogger>();
    }
    if (kind == "null") {
        return std::make_unique<NullLogger>();
    }
    if (kind.empty()) {
        return std::unexpected(CreateError::BadConfig);
    }
    return std::unexpected(CreateError::UnknownType);
}
#endif

}  // namespace fp

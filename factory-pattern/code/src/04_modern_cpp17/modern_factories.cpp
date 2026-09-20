#include "modern_factories.h"

namespace fp {

std::unique_ptr<LoggerCreatorBase> make_creator(std::string_view kind) {
    if (kind == "console") {
        return std::make_unique<LoggerCreator<ConsoleLogger>>();
    }
    if (kind == "null") {
        return std::make_unique<LoggerCreator<NullLogger>>();
    }
    return nullptr;
}

LoggerVariant make_variant_logger(LoggerKind kind) {
    switch (kind) {
        case LoggerKind::Console:
            return LoggerVariant{std::in_place_type<ConsoleLogger>};
        case LoggerKind::Null:
            return LoggerVariant{std::in_place_type<NullLogger>};
    }
    return LoggerVariant{std::in_place_type<NullLogger>};
}

void log_via_variant(LoggerVariant& variant, const std::string& message) {
    std::visit([&message](auto& logger) { logger.info(message); }, variant);
}

}  // namespace fp

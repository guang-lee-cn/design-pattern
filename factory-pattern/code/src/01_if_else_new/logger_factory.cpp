#include "logger_factory.h"

// 全工程里，只有本文件需要认识具体类。
// 这是工厂换来的东西，也是它的成本：创建逻辑被集中，同时被隔离。
#include "loggers.h"

#include <utility>

namespace fp {

std::unique_ptr<Logger> LoggerFactory::create(const std::string& kind) {
    if (kind == "console") {
        return std::make_unique<ConsoleLogger>();
    }

    if (kind == "file") {
        FileLoggerConfig config;
        config.path              = "fp_01_demo.log";
        config.append            = true;
        config.flush_interval_ms = 100;
        // encoding 用默认值 —— 调用方不必知道有这一项。
        return std::make_unique<FileLogger>(std::move(config));
    }

    if (kind == "null") {
        return std::make_unique<NullLogger>();
    }

    return nullptr;
}

std::vector<std::string> LoggerFactory::supported_kinds() {
    return {"console", "file", "null"};
}

}  // namespace fp

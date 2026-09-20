// ===========================================================================
// 反例：业务代码自己 if/else new
//
// 注意这个文件已经被收敛到最好 —— 每个调用点各自 inline 一段 if/else 的版本
// 只会更糟。本篇故意拿"最好的直接 new 版本"来对照，是为了让差距来自结构，
// 而不是来自写法水平。
// ===========================================================================

#include "order_flow.h"

// ↓ 反例的代价，就在这一行。
//   业务代码必须认识所有具体类，于是它和这些类绑定了编译依赖。
#include "loggers.h"

#include <memory>
#include <stdexcept>
#include <utility>

namespace fp {
namespace {

/// 这段 if/else 就是问题的全部。
///
/// 它回答了两个本该分开的问题：
///   「这个业务函数在做什么」和「日志器是怎么造出来的」。
std::unique_ptr<Logger> new_logger(const std::string& kind) {
    if (kind == "console") {
        return std::make_unique<ConsoleLogger>();
    }

    if (kind == "file") {
        // 构造细节暴露在业务代码里：
        // 文件路径、是否追加、刷盘间隔，全都要在这里拼出来。
        // FileLoggerConfig 一改，这里就要跟着改。
        FileLoggerConfig config;
        config.path              = "fp_01_demo.log";
        config.append            = true;
        config.flush_interval_ms = 100;
        return std::make_unique<FileLogger>(std::move(config));
    }

    if (kind == "null") {
        return std::make_unique<NullLogger>();
    }

    throw std::invalid_argument("unknown logger kind: " + kind);
}

}  // namespace

void run_order_flow(const std::string& logger_kind) {
    auto logger = new_logger(logger_kind);

    // ---- 以下 4 行与工厂版本逐字节相同 ----
    logger->info("order flow started");
    logger->info("validating payment request");
    logger->warn("retrying channel handshake");
    logger->info("order flow finished");
    // --------------------------------------
}

}  // namespace fp

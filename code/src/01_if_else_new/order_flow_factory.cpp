// ===========================================================================
// 正例：业务代码只调工厂
//
// 与 order_flow_naive.cpp 的差异，全部集中在上面那一行 include
// 和下面那两行创建语句上。其余部分逐字节相同。
// ===========================================================================

#include "order_flow.h"

// ↓ 正例拿到的东西：一个工厂的声明。
//   业务代码不知道 ConsoleLogger / FileLogger / NullLogger 的存在，
//   也不知道日志器需要哪些构造参数。
#include "logger_factory.h"

#include <memory>
#include <stdexcept>

namespace fp {

void run_order_flow(const std::string& logger_kind) {
    auto logger = LoggerFactory::create(logger_kind);
    if (!logger) {
        // 这一行不是"类型分支"，是错误处理 —— 两件事要分清。
        throw std::invalid_argument("unknown logger kind: " + logger_kind);
    }

    // ---- 以下 4 行与直接 new 版本逐字节相同 ----
    logger->info("order flow started");
    logger->info("validating payment request");
    logger->warn("retrying channel handshake");
    logger->info("order flow finished");
    // --------------------------------------------
}

}  // namespace fp

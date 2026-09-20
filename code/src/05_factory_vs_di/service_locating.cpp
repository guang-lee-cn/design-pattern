#include "service_locating.h"

#include "fp/logger.h"
#include "payment_factory.h"
#include "processors.h"

#include <memory>
#include <string>
#include <utility>

namespace fp {

ServiceLocatingOrderService::ServiceLocatingOrderService(std::string channel)
    : channel_(std::move(channel)) {}

PaymentResult ServiceLocatingOrderService::checkout(const PaymentRequest& request) {
    // 依赖在函数体里被就地创建。这一段就是问题的全部：
    //   - 想让测试用假实现，得先改这段代码或加一个全局开关；
    //   - 每次调用都新建 HttpClient，连接池、重试策略全部无从谈起；
    //   - OrderService 从此知道了工厂、知道了渠道名的来源。
    auto logger = std::make_shared<ConsoleLogger>();
    auto http   = std::make_shared<HttpClient>(logger);

    auto processor = PaymentProcessorFactory::create(channel_, http, logger);
    if (!processor) {
        return PaymentResult{false, "", "unsupported channel: " + channel_};
    }
    if (request.amount_cents <= 0) {
        logger->warn("reject non-positive amount for " + request.order_id);
        return PaymentResult{false, "", "invalid amount"};
    }
    return processor->pay(request);
}

}  // namespace fp

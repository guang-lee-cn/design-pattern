#include "order_service.h"

#include "fp/logger.h"

#include <string>
#include <utility>

namespace fp {

OrderService::OrderService(std::unique_ptr<PaymentProcessor> processor,
                           std::shared_ptr<Logger>           logger)
    : processor_(std::move(processor)), logger_(std::move(logger)) {}

PaymentResult OrderService::checkout(const PaymentRequest& request) {
    // 业务规则留在这里。
    if (request.amount_cents <= 0) {
        logger_->warn("reject non-positive amount for " + request.order_id);
        return PaymentResult{false, "", "invalid amount"};
    }

    // 渠道相关的一切都被推到了 processor_ 背后。
    // OrderService 不需要知道“怎么创建”它。
    logger_->info("order " + request.order_id + " -> " + processor_->channel());
    return processor_->pay(request);
}

}  // namespace fp

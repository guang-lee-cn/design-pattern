#include "processors.h"

#include "fp/logger.h"

#include <utility>

namespace fp {

// ---------------------------------------------------------------------------
// HttpClient：依赖注入的正例
// ---------------------------------------------------------------------------
HttpClient::HttpClient(std::shared_ptr<Logger> logger)
    : logger_(std::move(logger)) {}

std::string HttpClient::post(const std::string& url, const std::string& body) {
    ++call_count_;
    logger_->info("POST " + url + "  (" + std::to_string(body.size()) + " bytes)");
    return R"({"status":"ok"})";
}

// ---------------------------------------------------------------------------
// AlipayProcessor
// ---------------------------------------------------------------------------
AlipayProcessor::AlipayProcessor(std::shared_ptr<HttpClient> http,
                                 std::shared_ptr<Logger>     logger)
    : http_(std::move(http)), logger_(std::move(logger)) {}

std::string AlipayProcessor::channel() const {
    return "alipay";
}

PaymentResult AlipayProcessor::pay(const PaymentRequest& request) {
    const std::string body = "order=" + request.order_id +
                             "&amount=" + std::to_string(request.amount_cents);
    http_->post("https://api.alipay.example/v1/pay", body);
    logger_->info("alipay charged " + std::to_string(request.amount_cents) +
                  " cents for " + request.order_id);
    return PaymentResult{true, "ALIPAY-" + request.order_id, ""};
}

// ---------------------------------------------------------------------------
// WechatPayProcessor
// ---------------------------------------------------------------------------
WechatPayProcessor::WechatPayProcessor(std::shared_ptr<HttpClient> http,
                                       std::shared_ptr<Logger>     logger)
    : http_(std::move(http)), logger_(std::move(logger)) {}

std::string WechatPayProcessor::channel() const {
    return "wechat";
}

PaymentResult WechatPayProcessor::pay(const PaymentRequest& request) {
    const std::string body = "out_trade_no=" + request.order_id +
                             "&total_fee=" + std::to_string(request.amount_cents);
    http_->post("https://api.wechatpay.example/v3/pay", body);
    logger_->info("wechat charged " + std::to_string(request.amount_cents) +
                  " cents for " + request.order_id);
    return PaymentResult{true, "WECHAT-" + request.order_id, ""};
}

}  // namespace fp

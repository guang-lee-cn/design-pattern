#pragma once

#include "fp/payment.h"

#include <memory>
#include <string>

namespace fp {

class Logger;

/// 一个极简的“外部服务客户端”。
///
/// 它在本篇有两个作用：
///   1. 演示**依赖注入的正例**——它自己不创建 Logger，由构造函数送进来；
///   2. 演示**共享依赖**——HttpClient 和 OrderService 拿到的是同一个 Logger，
///      所以用 shared_ptr，而不是 unique_ptr。
///
/// 真实项目里它会是 HTTP / gRPC 客户端；这里只统计调用次数，便于断言。
class HttpClient {
public:
    explicit HttpClient(std::shared_ptr<Logger> logger);

    /// 返回响应体。demo 里不回真网络，只记账。
    std::string post(const std::string& url, const std::string& body);

    /// 被调用过几次，供测试断言。
    int call_count() const { return call_count_; }

private:
    std::shared_ptr<Logger> logger_;
    int                     call_count_ = 0;
};

/// 支付宝渠道。
class AlipayProcessor final : public PaymentProcessor {
public:
    AlipayProcessor(std::shared_ptr<HttpClient> http, std::shared_ptr<Logger> logger);

    std::string   channel() const override;
    PaymentResult pay(const PaymentRequest& request) override;

private:
    std::shared_ptr<HttpClient> http_;
    std::shared_ptr<Logger>     logger_;
};

/// 微信支付渠道。
class WechatPayProcessor final : public PaymentProcessor {
public:
    WechatPayProcessor(std::shared_ptr<HttpClient> http, std::shared_ptr<Logger> logger);

    std::string   channel() const override;
    PaymentResult pay(const PaymentRequest& request) override;

private:
    std::shared_ptr<HttpClient> http_;
    std::shared_ptr<Logger>     logger_;
};

}  // namespace fp

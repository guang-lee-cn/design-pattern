#pragma once

#include "fp/payment.h"

#include <memory>

namespace fp {

class Logger;

/// 订单服务——**依赖注入的正例**。
///
/// 它的依赖全部出现在构造签名里：想要什么，一眼可见。
/// 它不知道支付宝，不知道微信支付，不知道工厂，也不知道“渠道”这个概念
/// 是谁决定的。它只知道一件事：有人给了我一个 PaymentProcessor。
///
/// 两种依赖、两种所有权，是刻意区分的：
///
///   | 依赖        | 智能指针      | 理由                                   |
///   |-------------|---------------|----------------------------------------|
///   | processor   | unique_ptr    | 本服务独占它，别人不该拿到同一个实例   |
///   | logger      | shared_ptr    | 多个组件共享同一个日志器，生命周期独立 |
///
/// 构造签名同时就是这份类的“依赖清单”——这也是 DI 相对服务定位
/// 最容易被低估的好处：依赖成了编译期可见的契约。
class OrderService {
public:
    OrderService(std::unique_ptr<PaymentProcessor> processor,
                 std::shared_ptr<Logger>           logger);

    /// 真正的订单逻辑。注意 amount 的校验发生在这里——
    /// 它属于业务规则，不属于任何支付渠道。
    PaymentResult checkout(const PaymentRequest& request);

    /// 供演示与测试观察当前注入的实现。
    const PaymentProcessor& processor() const { return *processor_; }

private:
    std::unique_ptr<PaymentProcessor> processor_;
    std::shared_ptr<Logger>           logger_;
};

}  // namespace fp

#pragma once

#include <string>

namespace fp {

/// 支付请求（值类型，无所有权语义）。
struct PaymentRequest {
    std::string order_id;
    long long   amount_cents = 0;
};

/// 支付结果。
struct PaymentResult {
    bool        ok = false;
    std::string transaction_id;
    std::string error;
};

/// 支付渠道的抽象接口。
///
/// 注意这里**刻意只描述“创建出来之后要做什么”**，不描述“怎么创建”。
/// 怎么创建是工厂的职责——两者分开，正是本系列要讲清的第一条边界。
///
/// 接口故意保持最小：只有 channel() 与 pay()。
/// 抽象接口越小，用假实现替换它就越容易——这是单元测试能替身的前提。
class PaymentProcessor {
public:
    virtual ~PaymentProcessor() = default;

    /// 渠道名，用于日志与断言。
    virtual std::string channel() const = 0;

    /// 发起支付。成功时填充 transaction_id，失败时填充 error。
    virtual PaymentResult pay(const PaymentRequest& request) = 0;
};

}  // namespace fp

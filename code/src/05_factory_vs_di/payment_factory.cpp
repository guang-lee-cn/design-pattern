#include "payment_factory.h"

#include "processors.h"

#include <utility>

namespace fp {

std::unique_ptr<PaymentProcessor> PaymentProcessorFactory::create(
    const std::string&          channel,
    std::shared_ptr<HttpClient> http,
    std::shared_ptr<Logger>     logger) {
    // 这里就是“工厂”的全部内容：一个把类型名映射到构造调用的分支。
    // 它没有多态、没有注册表——本篇要讲清的是边界，不是工厂的花样。
    if (channel == "alipay") {
        return std::make_unique<AlipayProcessor>(std::move(http), std::move(logger));
    }
    if (channel == "wechat") {
        return std::make_unique<WechatPayProcessor>(std::move(http), std::move(logger));
    }
    return nullptr;
}

}  // namespace fp

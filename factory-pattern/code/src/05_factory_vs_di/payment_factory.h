#pragma once

#include "fp/payment.h"

#include <memory>
#include <string>

namespace fp {

class HttpClient;
class Logger;

/// 支付处理器工厂。
///
/// 它只回答两个问题：**创建哪个具体类**、**怎么创建**。
/// 它不回答“谁需要这个对象、怎么给它”——那是依赖注入的问题，
/// 由组合根（main.cpp）决定。
///
/// 两个刻意的设计选择：
///
///   1. **返回 `unique_ptr`**：所有权被显式转交出去。工厂造完就撒手，
///      不参与对象的生命周期管理，也不缓存实例。
///   2. **依赖（http / logger）由参数传入，而不是在工厂里 new**：
///      工厂若要自己创建依赖，就会退化成第二个组合根，
///      而 HttpClient 这类共享资源本该只建一份。
///
/// 创建失败返回 `nullptr`（本篇为保持示例最小而如此处理；
/// 工程代码里应改用 `std::optional` 或返回带错误码的结果类型）。
class PaymentProcessorFactory {
public:
    static std::unique_ptr<PaymentProcessor> create(
        const std::string&          channel,
        std::shared_ptr<HttpClient> http,
        std::shared_ptr<Logger>     logger);
};

}  // namespace fp

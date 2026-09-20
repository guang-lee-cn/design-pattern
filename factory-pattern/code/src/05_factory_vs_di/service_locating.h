#pragma once

#include "fp/payment.h"

#include <string>

namespace fp {

/// 订单服务——**服务定位的反例**。
///
/// 这个类“看起来”也能用，甚至看起来“用了工厂所以解耦了”。
/// 事实恰好相反。它有三个具体问题：
///
///   1. **构造签名看不出它需要 PaymentProcessor**。依赖藏进了函数体，
///      读签名的人以为它只需要一个字符串。
///   2. **无法在测试里替换实现**。要跑 checkout，就得跑真实的工厂、
///      真实的 HttpClient 构造路径——测试被迫覆盖与它无关的代码。
///   3. **依赖被反复重建**。每次 checkout 都新建一份 HttpClient 和 Logger，
///      共享资源在这里变成了逐次消耗品。
///
/// 第 8.4 节把这种写法叫“把工厂当 DI 用”。它的学名是 **服务定位
/// （Service Locating）**，在 DI 圈子里通常被当作反模式。
class ServiceLocatingOrderService {
public:
    explicit ServiceLocatingOrderService(std::string channel);

    PaymentResult checkout(const PaymentRequest& request);

private:
    std::string channel_;
};

}  // namespace fp

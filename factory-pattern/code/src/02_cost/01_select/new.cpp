// src/02_cost/01_select/new.cpp
//
// Same scenario with a factory: callers depend only on the abstract
// interface; the knowledge of "which concrete object, and how to build it"
// is concentrated in one place -- the factory.
#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string_view>
#include <utility>

#include "constants.h"

namespace {

// -- Abstract interface: the only type callers know --
// (Google style: interface classes carry no "I" prefix.)
class PaymentGateway {
public:
    virtual ~PaymentGateway()    = default;
    virtual void Pay(int amount) = 0;
};

// -- Concrete implementations hide behind the interface --
class WeChatPayGateway : public PaymentGateway {
public:
    void Pay(int amount) override {
        std::printf("[WeChat Pay] charged %d CNY\n", amount);
    }
};

class AlipayGateway : public PaymentGateway {
public:
    void Pay(int amount) override {
        std::printf("[Alipay]     charged %d CNY\n", amount);
    }
};

class UnionPayGateway : public PaymentGateway {
public:
    void Pay(int amount) override {
        std::printf("[UnionPay]   charged %d CNY\n", amount);
    }
};

// -- Factory: the single place that knows every concrete class and selects --
std::unique_ptr<PaymentGateway> CreatePaymentGateway(std::string_view channel) {
    if (channel == factory_pattern::kChannelWechat) {
        return std::make_unique<WeChatPayGateway>();
    }
    if (channel == factory_pattern::kChannelAlipay) {
        return std::make_unique<AlipayGateway>();
    }
    if (channel == factory_pattern::kChannelUnionPay) {
        return std::make_unique<UnionPayGateway>();
    }
    throw std::runtime_error("unknown payment channel");
}

// -- Business code: depends on the abstraction only; no concrete channel
// -- class name appears anywhere in it.
class OrderService {
public:
    explicit OrderService(std::unique_ptr<PaymentGateway> gateway) : gateway_(std::move(gateway)) {
    }

    void Checkout(int amount) {
        gateway_->Pay(amount);
    }

private:
    std::unique_ptr<PaymentGateway> gateway_;
};

} // namespace

int main() {
    using factory_pattern::kChannelAlipay;
    using factory_pattern::kChannelUnionPay;
    using factory_pattern::kChannelWechat;

    for (const std::string_view channel : {kChannelWechat, kChannelAlipay, kChannelUnionPay}) {
        OrderService service(CreatePaymentGateway(channel)); // composition point
        service.Checkout(100);
    }
}

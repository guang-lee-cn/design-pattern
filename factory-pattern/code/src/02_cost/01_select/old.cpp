// src/02_cost/01_select/old.cpp
//
// Scenario: order checkout with three payment channels:
// WeChat Pay, Alipay, UnionPay.
//
// Without a factory: the caller knows every concrete class by name and owns
// the "which channel" selection logic.
#include <cstdio>
#include <stdexcept>
#include <string>
#include <string_view>

#include "constants.h"

namespace {

// -- Three channels, no shared abstraction --
class WeChatPayGateway {
public:
    void Pay(int amount) {
        std::printf("[WeChat Pay] charged %d CNY\n", amount);
    }
};

class AlipayGateway {
public:
    void Pay(int amount) {
        std::printf("[Alipay]     charged %d CNY\n", amount);
    }
};

class UnionPayGateway {
public:
    void Pay(int amount) {
        std::printf("[UnionPay]   charged %d CNY\n", amount);
    }
};

// -- Business code: concrete names and selection logic live here --
class OrderService {
public:
    explicit OrderService(std::string_view channel) : channel_(channel) {
    }

    void Checkout(int amount) {
        // Cost 1: this code must name (and, in a real project, include)
        //        every concrete gateway.
        // Cost 2: adding a channel means editing this working business
        //        function -- it is not closed for modification.
        if (channel_ == factory_pattern::kChannelWechat) {
            WeChatPayGateway gateway;
            gateway.Pay(amount);
        } else if (channel_ == factory_pattern::kChannelAlipay) {
            AlipayGateway gateway;
            gateway.Pay(amount);
        } else if (channel_ == factory_pattern::kChannelUnionPay) {
            UnionPayGateway gateway;
            gateway.Pay(amount);
        } else {
            throw std::runtime_error("unknown payment channel");
        }
    }

private:
    std::string channel_;
};

} // namespace

int main() {
    using factory_pattern::kChannelAlipay;
    using factory_pattern::kChannelUnionPay;
    using factory_pattern::kChannelWechat;

    for (const std::string_view channel : {kChannelWechat, kChannelAlipay, kChannelUnionPay}) {
        OrderService service(channel);
        service.Checkout(100);
    }
}

// 组合根（Composition Root）。
//
// 整个程序里，「知道具体类型」的代码只允许出现在这一个文件里。
// 工厂在这里被调用一次，之后所有对象都通过构造函数拿到依赖——
// 这就是「工厂」和「依赖注入」在同一段代码里的分工。

#include "fp/logger.h"
#include "order_service.h"
#include "payment_factory.h"
#include "processors.h"
#include "service_locating.h"

#include <iostream>
#include <memory>
#include <string>

namespace {

fp::PaymentRequest order_with(long long cents) {
    return fp::PaymentRequest{"ORDER-20260916-001", cents};
}

void print_result(const char* label, const fp::PaymentResult& r) {
    std::cout << "  " << label
              << " -> ok=" << (r.ok ? "true" : "false")
              << "  txn=" << (r.transaction_id.empty() ? "-" : r.transaction_id)
              << "  err=" << (r.error.empty() ? "-" : r.error) << '\n';
}

// ---------------------------------------------------------------------------
// 反例
// ---------------------------------------------------------------------------
void demo_service_locating() {
    std::cout << "=== 反例：服务定位（OrderService 自己去要依赖） ===\n\n";
    std::cout << "  构造签名  ServiceLocatingOrderService(std::string channel)\n";
    std::cout << "            ↑ 看不出它实际需要一个 PaymentProcessor\n\n";

    fp::ServiceLocatingOrderService service("alipay");
    print_result("checkout", service.checkout(order_with(128800)));
    std::cout << "\n";
}

// ---------------------------------------------------------------------------
// 正例
// ---------------------------------------------------------------------------
void demo_dependency_injection() {
    std::cout << "=== 正例：组合根装配（工厂 + 依赖注入） ===\n\n";

    // ------------------------ 组合根开始 ------------------------
    // 1) 共享资源只建一次，用 shared_ptr 分发。
    //    logger 会被 HttpClient 和 OrderService 同时持有。
    auto logger = std::make_shared<fp::ConsoleLogger>();
    auto http   = std::make_shared<fp::HttpClient>(logger);

    // 2) 工厂回答：「创建哪个具体类、怎么创建」。
    //    返回值是 unique_ptr —— 所有权在工厂手里结束。
    const std::string channel = "alipay";
    auto processor = fp::PaymentProcessorFactory::create(channel, http, logger);
    // ------------------------ 组合根结束 ------------------------

    // 3) 依赖注入回答：「谁需要它、怎么给它」。
    //    这里没有 new，没有工厂调用，只有「把东西交出去」。
    fp::OrderService order_service(std::move(processor), logger);

    std::cout << "  构造签名  OrderService(unique_ptr<PaymentProcessor>, shared_ptr<Logger>)\n";
    std::cout << "            ↑ 依赖清单本身就是签名\n\n";

    print_result("checkout#1", order_service.checkout(order_with(128800)));
    print_result("checkout#2", order_service.checkout(
                                   fp::PaymentRequest{"ORDER-20260916-002", 5000}));

    std::cout << "\n  HttpClient 累计调用次数 = " << http->call_count() << "\n";
    std::cout << "  （两次下单共用同一个 HttpClient —— 依赖只被创建一次）\n\n";
}

}  // namespace

int main() {
    std::cout << "工厂 vs 依赖注入：同一条订单，两种装配方式\n";
    std::cout << "================================================\n\n";

    demo_service_locating();
    demo_dependency_injection();

    std::cout << "两段代码完成的是同一件事。差别不在结果，在职责归属：\n";
    std::cout << "  反例把依赖藏进函数体 —— 工厂被当成了取物窗口；\n";
    std::cout << "  正例把依赖写进构造签名 —— 工厂只负责创建，注入只负责传递。\n";
    return 0;
}

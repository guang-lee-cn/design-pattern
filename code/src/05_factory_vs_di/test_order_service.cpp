// OrderService 的单元测试。
//
// 这个文件里不会出现 PaymentProcessorFactory、HttpClient、AlipayProcessor。
// 原因很简单：OrderService 的构造签名只要求抽象接口。
// 这就是依赖注入换来的东西 —— 测试可以只关心被测对象本身。
//
// 本系列只演示「替换实现」这一件事，不展开测试策略（见系列边界声明）。

#include "fp/logger.h"
#include "order_service.h"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace {

int g_failures = 0;

void check(bool condition, const std::string& what) {
    std::cout << (condition ? "  [PASS] " : "  [FAIL] ") << what << '\n';
    if (!condition) {
        ++g_failures;
    }
}

fp::PaymentRequest order_with(long long cents) {
    return fp::PaymentRequest{"ORDER-TEST", cents};
}

/// 假实现 —— 整个类的有效代码不到 8 行。
/// 抽象接口越小，替身越廉价，这是接口设计直接换来的测试成本。
class RecordingProcessor final : public fp::PaymentProcessor {
public:
    std::string channel() const override { return "recording"; }

    fp::PaymentResult pay(const fp::PaymentRequest& request) override {
        calls.push_back(request.amount_cents);
        return fp::PaymentResult{true, "FAKE-TXN-1", ""};
    }

    std::vector<long long> calls;
};

/// 静默日志：测试不想往 stdout 打日志。
class SilentLogger final : public fp::Logger {
public:
    void info(const std::string&) override {}
    void warn(const std::string&) override {}
};

}  // namespace

int main() {
    std::cout << "OrderService 单元测试（只演示替换实现）\n\n";

    // ---- 用例 1：用假实现替换真实支付渠道 --------------------------------
    {
        auto  processor = std::make_unique<RecordingProcessor>();
        auto* observed  = processor.get();  // 仅用于观察，不接管所有权
        auto  logger    = std::make_shared<SilentLogger>();

        fp::OrderService service(std::move(processor), logger);
        const auto       result = service.checkout(order_with(128800));

        std::cout << "用例 1：替换实现\n";
        check(result.ok, "假实现返回成功");
        check(result.transaction_id == "FAKE-TXN-1", "结果确实来自注入的实现");
        check(observed->calls.size() == 1, "支付实现被调用了 1 次");
        check(observed->calls.at(0) == 128800, "传入实现的金额未被篡改");
        std::cout << '\n';
    }

    // ---- 用例 2：业务规则由 OrderService 自己负责 ------------------------
    {
        auto  processor = std::make_unique<RecordingProcessor>();
        auto* observed  = processor.get();
        auto  logger    = std::make_shared<SilentLogger>();

        fp::OrderService service(std::move(processor), logger);
        const auto       result = service.checkout(order_with(0));

        std::cout << "用例 2：非法输入不会穿透到渠道层\n";
        check(!result.ok, "零金额被拒绝");
        check(observed->calls.empty(), "非法请求未触达支付实现");
        std::cout << '\n';
    }

    // ---- 用例 3：服务持有的确实是我们给的那个实现 ------------------------
    {
        auto  processor = std::make_unique<RecordingProcessor>();
        auto  logger    = std::make_shared<SilentLogger>();

        fp::OrderService service(std::move(processor), logger);

        std::cout << "用例 3：依赖就是注入进来的那一个\n";
        check(service.processor().channel() == "recording",
              "channel() 返回注入实现的标识");
        std::cout << '\n';
    }

    if (g_failures == 0) {
        std::cout << "全部通过\n";
        return 0;
    }
    std::cout << "失败 " << g_failures << " 项\n";
    return 1;
}

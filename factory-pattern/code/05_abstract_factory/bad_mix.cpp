// 05-抽象工厂：bad_mix.cpp —— 产品族的配套约束被打破时会发生什么
// 编译：g++ -std=c++17 -Wall bad_mix.cpp -o bad && ./bad
//
// 场景：芯片平台 A / B，各自提供 UART 与 GPIO。
// 两者在同一个"平台"内是配套的：UART 的引脚复用寄存器基址与 GPIO 的
// 寄存器布局必须来自同一平台，否则配置写到错误地址、UART 收不到数据。
//
// 本文件演示：用"每类产品一个独立工厂"的思路，调用方可以合法地
// 混搭出 A 的 UART + B 的 GPIO —— 编译器不拦，运行期炸。

#include <cstdio>
#include <memory>
#include <stdexcept>
#include <string>

// ---------- 产品接口 ----------

class Uart {
public:
    virtual ~Uart() = default;
    virtual std::string hw_id() = 0;          // 所属平台标识
    virtual std::string pinmux_base() = 0;    // 引脚复用寄存器基址
};

class Gpio {
public:
    virtual ~Gpio() = default;
    virtual std::string hw_id() = 0;
    virtual std::string reg_base() = 0;
};

// ---------- 平台 A ----------

class UartA : public Uart {
public:
    std::string hw_id() override { return "A"; }
    std::string pinmux_base() override { return "0x40001000"; }
};
class GpioA : public Gpio {
public:
    std::string hw_id() override { return "A"; }
    std::string reg_base() override { return "0x40002000"; }
};

// ---------- 平台 B ----------

class UartB : public Uart {
public:
    std::string hw_id() override { return "B"; }
    std::string pinmux_base() override { return "0x50001000"; }
};
class GpioB : public Gpio {
public:
    std::string hw_id() override { return "B"; }
    std::string reg_base() override { return "0x50002000"; }
};

// ---------- "每类产品一个独立工厂"——混搭在这里成为可能 ----------

std::unique_ptr<Uart> create_uart(const std::string& platform) {
    if (platform == "A") return std::make_unique<UartA>();
    if (platform == "B") return std::make_unique<UartB>();
    throw std::invalid_argument("unknown platform: " + platform);
}

std::unique_ptr<Gpio> create_gpio(const std::string& platform) {
    if (platform == "A") return std::make_unique<GpioA>();
    if (platform == "B") return std::make_unique<GpioB>();
    throw std::invalid_argument("unknown platform: " + platform);
}

// ---------- 驱动代码：把 UART 的引脚复用配置写下去 ----------

void uart_configure_pin(Uart& uart, Gpio& gpio) {
    std::printf("uart  hw=%s pinmux_base=%s\n", uart.hw_id().c_str(), uart.pinmux_base().c_str());
    std::printf("gpio  hw=%s reg_base   =%s\n", gpio.hw_id().c_str(), gpio.reg_base().c_str());

    // 配套约束的检查：只有同平台的 GPIO 才能正确响应 UART 的引脚配置
    if (uart.hw_id() != gpio.hw_id()) {
        throw std::runtime_error("pinmux 写入平台不匹配的 GPIO：配置丢失，UART 收不到数据");
    }
    std::printf("=> 引脚复用配置成功\n");
}

int main() {
    std::printf("=== 场景 1：同平台组合（配置成功）===\n");
    auto uart_a = create_uart("A");
    auto gpio_a = create_gpio("A");
    try {
        uart_configure_pin(*uart_a, *gpio_a);
    } catch (const std::exception& e) {
        std::printf("error: %s\n", e.what());
    }

    std::printf("\n=== 场景 2：混搭（编译器完全不拦，运行期炸）===\n");
    // 注意：这两行写法上没有任何异常——都是合法的工厂调用
    auto uart_a2 = create_uart("A");     // 从 A 平台拿 UART
    auto gpio_b  = create_gpio("B");     // 从 B 平台拿 GPIO —— 混了
    try {
        uart_configure_pin(*uart_a2, *gpio_b);
    } catch (const std::exception& e) {
        std::printf("error: %s\n", e.what());
    }

    std::printf("\n判据：错误在【运行期】才暴露，且暴露点离犯错点很远（装配处 vs 配置处）。\n");
    return 0;
}

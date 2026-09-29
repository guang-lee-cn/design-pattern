// 05-抽象工厂：abstract_factory.cpp —— 用"族工厂"把配套约束变成结构约束
// 编译：g++ -std=c++17 -Wall abstract_factory.cpp -o af && ./af
//
// 对照 bad_mix.cpp：混搭之所以可能，是因为"选平台"这件事被拆成了
// 每个产品各自的工厂调用（create_uart("A") / create_gpio("B")）。
// 抽象工厂把"选平台"收敛成【选一个工厂对象】，此后该工厂产出的所有产品
// 天然同族 —— 混搭在调用侧无处表达。

#include <cstdio>
#include <memory>
#include <string>

// ---------- 产品接口（三件套） ----------

class Uart {
public:
    virtual ~Uart() = default;
    virtual std::string hw_id() = 0;
    virtual std::string pinmux_base() = 0;
};

class Gpio {
public:
    virtual ~Gpio() = default;
    virtual std::string hw_id() = 0;
    virtual std::string reg_base() = 0;
};

class Timer {
public:
    virtual ~Timer() = default;
    virtual std::string hw_id() = 0;
    virtual std::string tick_hz() = 0;
};

// ---------- 平台 A 的整族产品 ----------

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
class TimerA : public Timer {
public:
    std::string hw_id() override { return "A"; }
    std::string tick_hz() override { return "1000000"; }
};

// ---------- 平台 B 的整族产品 ----------

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
class TimerB : public Timer {
public:
    std::string hw_id() override { return "B"; }
    std::string tick_hz() override { return "24000000"; }
};

// ===========================================================================
// 抽象工厂：一个接口，三个工厂方法
// ===========================================================================
// 关键：不是抽象"产品"，而是抽象"整族产品"。调用方持有的是 BspFactory&，
// 拿到的 UART/GPIO/Timer 必定同族 —— 因为三者都出自同一个对象。
class BspFactory {
public:
    virtual ~BspFactory() = default;
    virtual std::unique_ptr<Uart>  create_uart() = 0;
    virtual std::unique_ptr<Gpio>  create_gpio() = 0;
    virtual std::unique_ptr<Timer> create_timer() = 0;
    virtual std::string name() = 0;
};

// ---------- 具体工厂：一族一个 ----------

class BspFactoryA : public BspFactory {
public:
    std::unique_ptr<Uart>  create_uart()  override { return std::make_unique<UartA>(); }
    std::unique_ptr<Gpio>  create_gpio()  override { return std::make_unique<GpioA>(); }
    std::unique_ptr<Timer> create_timer() override { return std::make_unique<TimerA>(); }
    std::string name() override { return "chip-A"; }
};

class BspFactoryB : public BspFactory {
public:
    std::unique_ptr<Uart>  create_uart()  override { return std::make_unique<UartB>(); }
    std::unique_ptr<Gpio>  create_gpio()  override { return std::make_unique<GpioB>(); }
    std::unique_ptr<Timer> create_timer() override { return std::make_unique<TimerB>(); }
    std::string name() override { return "chip-B"; }
};

// ===========================================================================
// 平台无关的驱动代码：只认识 BspFactory&，不认识任何具体平台
// ===========================================================================
void bring_up(BspFactory& f) {
    auto uart  = f.create_uart();
    auto gpio  = f.create_gpio();
    auto timer = f.create_timer();

    std::printf("--- %s ---\n", f.name().c_str());
    std::printf("uart  hw=%s pinmux=%s\n", uart->hw_id().c_str(), uart->pinmux_base().c_str());
    std::printf("gpio  hw=%s reg   =%s\n", gpio->hw_id().c_str(), gpio->reg_base().c_str());
    std::printf("timer hw=%s tick  =%s Hz\n", timer->hw_id().c_str(), timer->tick_hz().c_str());

    // 一致性断言：三个产品必然同族——这是结构保证，不是运气
    if (uart->hw_id() == gpio->hw_id() && gpio->hw_id() == timer->hw_id()) {
        std::printf("=> 全族一致（hw_id 均为 %s），混搭在调用侧无法表达\n", uart->hw_id().c_str());
    } else {
        std::printf("=> 不一致！这在抽象工厂下不可能发生\n");
    }
}

int main() {
    BspFactoryA fa;
    BspFactoryB fb;

    bring_up(fa);
    std::printf("\n");
    bring_up(fb);

    std::printf("\n注意：bring_up() 一份代码适配两个平台，编译期不认识 UartA/UartB。\n");
    return 0;
}

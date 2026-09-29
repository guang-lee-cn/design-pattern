# 判断题 01：该不该上工厂

> **目的**：检验「判断力」，不是背定义。三题分别对应三个决策档位——不上 / 停在简单工厂 / 升到抽象工厂。
>
> **作答方式**：对每题给出 ① 结论 ② 一句理由（引用三痛点或升级判据即可，不用长篇）。
>
> **参考答案在文末折叠区**，建议先自答再展开。

---

## 题 1 —— 单点创建

```cpp
// logger.h
class Logger {
public:
    virtual ~Logger() = default;
    virtual void write(const std::string& msg) = 0;
};

class FileLogger : public Logger {
public:
    explicit FileLogger(const std::string& path);
    void write(const std::string& msg) override;
};

// 全项目搜索：Logger 只有 FileLogger 这一个派生类
```

```cpp
// main.cpp
int main() {
    // TODO: 将来支持网络日志，可能要加 NetworkLogger
    FileLogger logger("/var/log/collector.log");
    run(logger);                        // void run(Logger&)
}
```

**现状**：`FileLogger` 的构造全项目只出现这一行；`run()` 接的是 `Logger&`，业务代码只认识基类。

**问题**：要不要加一个 `create_logger()` 工厂函数，把创建收口？

### 我的作答

- 结论：
- 理由：

---

## 题 2 —— 三处拷贝，但产品集合稳定

**背景**：全项目 4 种传感器，`temp` / `pressure` / `humidity` / `flow`。**这 4 种已经两年没变过**，产线硬件固定，不会再加新款。

```cpp
// collector.cpp
std::unique_ptr<Sensor> make_sensor(const std::string& type) {
    if (type == "temp")     return std::make_unique<TempSensor>(25.0);
    if (type == "pressure") return std::make_unique<PressureSensor>(110);
    if (type == "humidity") return std::make_unique<HumiditySensor>();
    if (type == "flow")     return std::make_unique<FlowSensor>(0.5);
    return nullptr;
}
```

```cpp
// selftest.cpp —— 为了不走虚函数做快速自检，拷了一份同样的 if-else
//                  但漏掉了 flow 这一支
std::unique_ptr<Sensor> make_sensor_fast(const std::string& type) { ... }
```

```cpp
// diag.cpp —— 为了打诊断信息，又拷了一份
//              pressure 的默认量程写成了 100（collector 那边是 110）
std::unique_ptr<Sensor> make_sensor_diag(const std::string& type) { ... }
```

**问题**：这份代码要不要改？如果改，改到哪一层为止？

### 我的作答

- 结论：
- 理由：

---

## 题 3 —— 三个独立工厂函数

```cpp
struct Uart  { virtual ~Uart()  = default; virtual void send(const char*) = 0; };
struct Gpio  { virtual ~Gpio()  = default; virtual void set(int pin, int lvl) = 0; };
struct Timer { virtual ~Timer() = default; virtual void sleep_ms(int ms) = 0; };

// platform 来自运行期配置 /etc/hw.conf，取值 "A" 或 "B"
std::unique_ptr<Uart>  create_uart (const std::string& platform);
std::unique_ptr<Gpio>  create_gpio (const std::string& platform);
std::unique_ptr<Timer> create_timer(const std::string& platform);

void app_init(const std::string& plat) {
    auto u = create_uart(plat);
    auto g = create_gpio(plat);        // ← 这一行的参数如果被人改掉会怎样？
    auto t = create_timer(plat);
    // ...
}
```

**硬约束**：A 平台 UART 的 pinmux 寄存器落在 GPIO 寄存器组内偏移 `0x10`；B 平台寄存器布局不同。**两平台的部件混搭会导致静默的数据丢失**。

**已知**：平台 A、B 已存在，**预计还要加 C、D**。

**问题**：这三个独立的工厂函数有什么隐患？该怎么改？

### 我的作答

- 结论：
- 理由：

---

<details>
<summary><b>参考答案（先自答再展开）</b></summary>

### 题 1 答案：**不要上工厂**

三个痛点**全部不成立**：

| 痛点 | 检查 |
|---|---|
| 1 编译耦合 | 只有一个 TU 用 `FileLogger`，业务侧已经只认 `Logger&` |
| 2 修改扩散 | 创建决策只有 **1 处**。没有第 2 份拷贝，就没有扩散 |
| 3 构造知识泄漏 | `path` 是**调用方自己的信息**，不是产品体系的知识 |

加 `create_logger()` 之后，`main` 里从 `FileLogger logger(path)` 变成 `logger = create_logger(path)`——**多一层间接，什么都没买到**。

**陷阱**：那行 `// TODO: 将来支持网络日志` 是干扰项。「将来可能」不是判据。真到要加的时候，改动量是「加一个类 + 改 `main` 里一行」，而上工厂**并不能让这一步变简单**。等真出现第 2 个创建点时再收口，成本不变。

> 判据：具体类名在 ≥2 个不相关 TU 的创建点出现 → 才值得上。这里只有 1 个。

---

### 题 2 答案：**该收口，但只到简单工厂为止——不要再往工厂方法升**

**该收口的部分**（痛点 1、2 确实成立）：

- 3 份拷贝，而且已经**烂掉了**：`selftest` 漏了 `flow`，`diag` 的 `pressure` 量程和主版本不一致（110 vs 100）
- 这不是理论风险，是**已经发生的错误**——而且编译器完全不管
- 收口动作：3 份 → 1 个 `make_sensor()`，业务 TU 只 include 抽象基类的头

**不该升级的部分**：

产品集合**封闭且两年零变化**——这正是第 2 节判据 3「停在原地」的教科书信号。升到工厂方法要付：4 个产品类 + 4 个工厂类 + 装配点改写 ≈ 9 个类的代价，去买一个**不存在的变化热点**。

**这题的关键是两头都对**：不动（让 3 份拷贝继续漂）是错的，升过头（上工厂方法）也是错的。

**顺带一个值得讨论的点**：`return nullptr` 这个分支。调用方每个地方都要判空，一旦漏判就是解引用崩溃。两种改法——抛异常（明确契约）或返回 `NullSensor` 空对象（调用方无感知，`read()` 返回 NaN）。属于附赠考点，不算硬性对错。

> 一句话：**简单工厂治「扩散」，但扩散收敛完了就该停；升级要有「高频改动」这个理由，不能靠「反正更解耦」。**

---

### 题 3 答案：**该上抽象工厂**

**隐患**：三个函数**各自独立接收 `platform`**，所以调用方**能表达出混搭**：

```cpp
auto u = create_uart("A");
auto g = create_gpio("B");     // 语法完全合法，编译器一声不吭
```

后果：`uart` 的 pinmux 写向一个 B 平台布局的 GPIO 寄存器组，**静默数据丢失**——能编译、能启动、跑一阵才炸，而且暴露点离犯错点很远。这就是第 5 节 `bad_mix.cpp` 演示的场景。

**改法**：把「整族」抽象出来，`create_uart` / `create_gpio` / `create_timer` 变成 `platform_factory` 的**纯虚成员**；`platform_a_factory` / `platform_b_factory` 各实现一整套。

调用方拿一个工厂对象，三个部件都从**同一个对象**拿 → **混搭在类型系统里无法表达**。约束从「记得不要混搭」（人肉纪律）变成「混搭写不出来」（结构保证）。

**为什么现在就该升**（而不像题 2 那样停住）：

- 产品之间确实存在**「必须配套」的约束** → 判据 2 成立
- 「平台还要加 C、D」→ 加**族** = 加一个新类，0 改旧码，正是抽象工厂的舒适区

**陷阱 & 要认的代价**：

「`platform` 来自运行期配置文件」**不是**反对理由。抽象工厂不要求编译期确定族——运行期用一个 switch / 注册表选出工厂对象即可，选完照样享受族一致性。

但反过来必须认：如果将来要加 **SPI 这个新产品类型**，就要改接口 + N 个具体工厂（第 5 节实测：编译错误增量 = 族数，线性）。所以上之前先确认**产品类型集合相对稳定，只有族在长**。

> 一句话：**抽象工厂只为「族的配套约束」付费；产品类型本身还在长的时候，它是负债不是资产。**

</details>

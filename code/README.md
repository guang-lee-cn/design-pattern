# 《C++ 工厂模式：从 if/else 到架构边界》配套代码

本仓库是系列文章的配套代码。所有代码均可独立编译运行，文中代码块与这里的文件同名同序。

## 构建

需要 CMake ≥ 3.16，以及支持 C++17 的编译器（GCC 11+ / Clang 14+ / MSVC 19.3+）。

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

`src/CMakeLists.txt` 会自动跳过尚未编写的篇章，因此新增篇章时无需改动任何已有文件。

## 目录结构

```
factory-pattern/
├── CMakeLists.txt                    # 顶层：fp_common（接口）+ fp_core（共享实现）
├── include/fp/                       # 全系列共享的抽象接口
│   ├── logger.h                      #   Logger / ConsoleLogger / NullLogger
│   └── payment.h                     #   PaymentProcessor（第 5 篇起使用）
└── src/
    ├── fp/logger.cpp                 # 共享接口的非内联实现
    ├── 01_if_else_new/               # 第 1 篇《为什么你的 C++ 代码里到处都是 if/else new》
    │   ├── main.cpp                  #   被两个可执行文件逐字节共用
    │   ├── order_flow.h              #   业务代码的入口声明（无版本痕迹）
    │   ├── order_flow_naive.cpp      #   反例：自己 if/else new
    │   ├── order_flow_factory.cpp    #   正例：只调 LoggerFactory
    │   ├── logger_factory.h/.cpp     #   工厂：唯一认识具体类的翻译单元
    │   └── loggers.h/.cpp            #   具体日志器 + 会变化的构造参数
    ├── 04_modern_cpp17/              # 第 4 篇《C++17 之后，工厂模式发生了哪些变化》
    │   ├── probe_unique_ptr.cpp      #   §7.2 三问的实证（含两条 static_assert）
    │   ├── modern_factories.h/.cpp   #   模板工厂 / if constexpr / variant
    │   ├── expected_compat.h/.cpp    #   optional 主线 + C++23 expected 条件编译
    │   ├── self_register.h/.cpp      #   注册表 + 自注册句柄
    │   ├── plugins/plugin_stdout.cpp #   插件：只负责静态初始化时注册自己
    │   ├── main_self_register.cpp    #   两个链接方式共用的 main
    │   └── main.cpp                  #   总演示
    └── 05_factory_vs_di/             # 第 5 篇《工厂和依赖注入，到底谁管什么》
        ├── main.cpp                  #   组合根：工厂 + 依赖注入
        ├── order_service.h/.cpp      #   正例：构造注入
        ├── service_locating.h/.cpp   #   反例：服务定位
        ├── processors.h/.cpp         #   HttpClient / 支付宝 / 微信支付实现
        ├── payment_factory.h/.cpp    #   PaymentProcessorFactory
        └── test_order_service.cpp    #   单元测试：只演示替换实现
```

## 各篇的可执行文件

```bash
# 第 1 篇：同一份 main，两个版本，输出逐字节相同
./build/src/01_if_else_new/fp_01_naive   --kind console
./build/src/01_if_else_new/fp_01_factory --kind console

# 第 1 篇：两个版本的编译依赖对比（反例的依赖列表里会出现 loggers.h）
g++ -std=c++17 -I src -I include -MM src/01_if_else_new/order_flow_naive.cpp
g++ -std=c++17 -I src -I include -MM src/01_if_else_new/order_flow_factory.cpp

# 第 4 篇：§7.2 三问
./build/src/04_modern_cpp17/fp_04_probe_unique_ptr

# 第 4 篇：同一个插件，两种链接方式
./build/src/04_modern_cpp17/fp_04_self_register_dropped
./build/src/04_modern_cpp17/fp_04_self_register_fixed

# 第 4 篇：总演示
./build/src/04_modern_cpp17/fp_04_main

# 第 5 篇
./build/src/05_factory_vs_di/fp_05_main
```

## 篇章与代码对应

| 篇 | 标题 | 目录 | 状态 |
|---|---|---|---|
| 1 | 为什么你的 C++ 代码里到处都是 if/else new | `01_if_else_new/` | **已完成** |
| 2 | 简单工厂、工厂方法、抽象工厂：不是三个并列模式 | `02_three_factories/` | 待编写 |
| 3 | 抽象工厂实战：产品族、组合根与自注册 | `03_assembly/` | 待编写 |
| 4 | C++17 之后，工厂模式发生了哪些变化 | `04_modern_cpp17/` | **已完成** |
| 5 | 工厂和依赖注入，到底谁管什么 | `05_factory_vs_di/` | **已完成** |
| 6 | 什么时候不该用工厂：过度设计的信号 | `06_when_not_to_use/` | 待编写 |
| 7 | 从 Fast DDS 到 Nginx：开源项目里的工厂模式 | `07_read_the_sources/` | 待编写 |
| 8 | 系列索引 | — | 待上线 |

## 关于四处刻意的结构选择

**一、共享接口做成静态库，而不是 header-only。**
`fp_core` 是静态库，不是 `INTERFACE` 库。这样第 1 篇才能真实演示"改一个实现要重编译多少翻译单元"，第 4 篇才能演示"链接器会丢弃未被引用的 `.cpp`"——这两个坑在 header-only 结构里根本复现不出来。

**二、第 1 篇的两个可执行文件共用同一份 `main.cpp`。**
不是副本，是同一个路径。`fp_01_naive` 与 `fp_01_factory` 的全部差异集中在 `order_flow_naive.cpp` / `order_flow_factory.cpp` 这一个翻译单元里，其余源文件逐字节相同。

**三、第 4 篇的同一个插件被编译进两种库。**
`fp_04_plugins_static`（STATIC）与 `fp_04_plugins_object`（OBJECT）来自同一个 `plugins/plugin_stdout.cpp`。前者会被链接器丢掉，后者不会——这就是自注册那个坑的复现装置。

**四、第 5 篇的测试目标不链接工厂库。**
`fp_05_test_order_service` 只链接 `order_service.cpp` 与 `fp_core`，**不**链接 `fp_05_factory`。因为 `OrderService` 的构造函数只要求抽象接口，它不需要工厂。这不是省略，是一句可以编译、也可以被 `nm` 验证的断言：

```bash
nm -C build/src/05_factory_vs_di/fp_05_test_order_service | grep -E "Factory|Alipay|HttpClient"
# 无输出
```

## 边界声明

本系列讲的是**创建与装配的职责划分**。不覆盖线程安全、异常安全、ABI 兼容；测试只演示"替换实现"这一件事，不展开测试策略。

## 编译器矩阵

已验证：GCC 13.3.0（Ubuntu 24.04, x86-64）。Clang 与 MSVC 的验证随对应篇章补充；第 4 篇给出了 MSVC + 静态库下自注册被链接器裁剪的机制与三条对策，但**尚未在 MSVC 上实测**——这一条属于待验证项，读者若在 MSVC 上跑出不同结果，以实测为准。

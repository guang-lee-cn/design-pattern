# design-pattern

C++ 设计模式实践库。**一个模式一个自包含文件夹**——演化背景、知识拆解、可编译代码、问题复盘都在同一处，互不依赖。

## 目录约定

```
design-pattern/
├── TEMPLATE.md        抽象基类：新模式的骨架与知识拆解流程（写新模式前读这份）
├── tools/             机械校验脚本
└── <pattern-name>/    一个模式的实现
    ├── README.md      该模式的导航（从哪读起）
    ├── docs/          正文，一篇文章一个文件（markdown + 内嵌 Mermaid）
    ├── code/          配套 C++ 工程，独立可编译
    └── notes/         问题记录与复盘
```

新增一个模式 = 新建一个同名文件夹 + 照 [`TEMPLATE.md`](TEMPLATE.md) 填。

**知识进仓库，过程出仓库。** 进度、决策记录、系列排期、原始素材都留在仓库外——公开仓库只承载结论，不承载到达结论的路径。

## 模式清单

| 目录 | 分类 | 状态 |
|---|---|---|
| [`factory-pattern/`](factory-pattern/) | 创建型 | 进行中（3/8 篇） |

## 快速开始

```bash
cd factory-pattern/code
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

需要 CMake ≥ 3.16，以及支持 C++17 的编译器（GCC 9+ / Clang 10+ / MSVC 19.20+）。

## 复核

```bash
python3 tools/check_pattern.py factory-pattern
```

按 [`TEMPLATE.md`](TEMPLATE.md) 检查结构齐全、序号命名、md 相对链接、正文声明的代码路径、**代码块来源标注**。只用 `python3` 标准库。

## 体例约定

- **代码块逐块标注来源。** 出自配套工程的标 `// src/<子目录>/<文件>（节选）`（路径相对该模式的 `code/`）；示意代码标 `// 示意：…，本段不在配套仓库中`；引用标准库的标 `// 标准库签名（摘自 <头文件>），非本仓库代码`。读者据此判断哪一段能直接拿去编译。
- **涉及 C++ 标准版本差异的论断，必须落成一个能被编译器检查的装置**，不能只写在正文里。起因见 [`factory-pattern/notes/`](factory-pattern/notes/)。
- **图统一用 Mermaid 内嵌**，不依赖任何外部渲染服务。
- **开源案例必须回源核验**：能指出真实文件与函数名，不接受转述。

## 协作

读取本仓库的编码助手请先看 [`AGENTS.md`](AGENTS.md)；进度锚点是 [`STATE.md`](STATE.md)。

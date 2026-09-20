# design-pattern

C++ 设计模式实践库。**一个模式一个自包含文件夹**——演化背景、知识拆解、可编译代码、问题复盘都在同一处，互不依赖。

## 目录约定

```
<pattern-name>/
├── README.md     该模式的导航（从哪读起）
├── plan/         系列内容大纲与路线图
├── docs/         正文（markdown + 内嵌 Mermaid）
├── code/         配套 C++ 工程，独立可编译
├── notes/        问题记录与复盘
└── refs/         素材与溯源，只读
```

新增一个模式 = 新建一个同名文件夹。模式之间不共享代码，也不共享构建。

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

## 体例约定

- **代码块逐块标注来源。** 出自配套工程的标 `// <pattern>/code/<路径>`，示意代码标"不在配套工程中"。读者据此判断哪一段能直接拿去编译。
- **涉及 C++ 标准版本差异的论断，必须落成一个能被编译器检查的装置**，不能只写在正文里。起因见 [`factory-pattern/notes/`](factory-pattern/notes/)。
- **图统一用 Mermaid 内嵌**，不依赖任何外部渲染服务。
- `refs/` 是只读溯源素材；与正文冲突时以正文为准，冲突记入对应模式的 `notes/`。

## 协作

读取本仓库的编码助手请先看 [`AGENTS.md`](AGENTS.md)；进度锚点是 [`STATE.md`](STATE.md)。

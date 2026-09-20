# factory-pattern · C++ 工厂模式

> 创建型 · 8 篇系列 · 进行中（3/8）

## 从哪读起

| 入口 | 内容 |
|---|---|
| [`plan/`](plan/) | 系列内容大纲与路线图：整个系列打算讲什么、按什么顺序 |
| [`docs/`](docs/) | 已完成正文（markdown + 内嵌 Mermaid） |
| [`code/`](code/) | 配套 C++ 工程，随篇章增量增长，可独立编译 |
| [`notes/`](notes/) | 写作过程中的问题记录与复盘 |
| [`refs/`](refs/) | 素材与溯源，只读 |

## 正文

| # | 标题 | 主题 |
|---|---|---|
| 1 | [为什么你的 C++ 代码里到处都是 if-else-new](docs/01-为什么你的C++代码里到处都是if-else-new.md) | 症状与代价 |
| 4 | [C++17 之后，工厂模式发生了哪些变化](docs/04-C++17之后，工厂模式发生了哪些变化.md) | 现代 C++ 的重写 |
| 5 | [工厂和依赖注入，到底谁管什么](docs/05-工厂和依赖注入，到底谁管什么.md) | 边界划分 |

计划共 8 篇（7 篇正文 + 1 篇索引），章节大纲与依赖顺序见
[`plan/工厂模式Blog章节与计划.md`](plan/工厂模式Blog章节与计划.md)。

## 编译

```bash
cd code
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## 工程结构

```
code/
├── CMakeLists.txt        顶层：C++17，自动纳入 src/ 下的子目录
├── include/fp/           跨篇共享的头（Logger 等）
└── src/
    ├── fp/               共享实现
    ├── 01_if_else_new/   第 1 篇
    ├── 04_modern_cpp17/  第 4 篇
    └── 05_factory_vs_di/ 第 5 篇
```

`src/CMakeLists.txt` 按子目录存在性自动纳入——**新增篇章不需要改动任何已有文件**。

## 已知问题

见 [`notes/`](notes/)。

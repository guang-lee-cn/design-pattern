# design-pattern

C++ 设计模式实践库。**一个模式一个自包含文件夹**——演化背景、知识拆解、可编译代码、问题复盘都在同一处，互不依赖。

## 目录约定

```
design-pattern/
├── AGENTS.md          协作约定（人机分工、续传协议、内容体例）
├── TEMPLATE.md        抽象基类：新模式的骨架与知识拆解流程（写新模式前读这份）
├── tools/             机械校验脚本（check_pattern.py / check_state.py）
├── STATE.md           进度锚点（恢复会话只对齐、不产出）
└── <pattern-name>/    一个模式的实现
    ├── README.md      该模式的导航（从哪读起、已知问题在哪）
    ├── docs/          正文，一篇文章一个文件（markdown + 内嵌 Mermaid + 手写 SVG/GIF）
    │   └── assets/    手写 SVG 具象图与 GIF 动图
    ├── code/          配套示例，每个场景一个自包含子目录
    └── notes/         问题记录与复盘（可选：真踩过坑才建）
```

新增一个模式 = 新建一个同名文件夹 + 照 [`TEMPLATE.md`](TEMPLATE.md) 填。

**知识进仓库，过程出仓库。** 进度、决策记录、系列排期、原始素材都留在仓库外——公开仓库只承载结论，不承载到达结论的路径。

## 模式清单

| 目录 | 分类 | 状态 |
|---|---|---|
| [`factory-pattern/`](factory-pattern/) | 创建型 | 正文 17 篇（主线 7 + 子专题 2 + 专题 6 + 检验 1）+ 13 个可运行场景 |

## 快速开始

**没有工程根统一构建**——按场景跑，每个场景自带入口：

```bash
bash factory-pattern/code/01_birth/build.sh      # 第 1 节：三痛点 + 可测试性对照（含 gcov）
bash factory-pattern/code/A06_plugin/build.sh    # A06：dlopen 插件，五组实验
cd factory-pattern/code/03_simple_factory/multi_tu && bash run.sh   # 03：多 TU 重建范围实测
```

单文件示例不必走脚本，正文里直接给了命令（`g++ -std=c++17 -Wall xxx.cpp -o /tmp/x && /tmp/x`）。

需要支持 C++17 的编译器（GCC 9+ / Clang 10+）；个别场景另需 `-fsanitize=address` 或 CMake ≥ 3.16。

## 复核

```bash
python3 tools/check_pattern.py factory-pattern   # 按 TEMPLATE.md 复核这个模式
python3 tools/check_state.py . --resume          # 打印进度状态卡
```

`check_pattern.py` 检查结构齐全、`code/` 构建入口、docs 三层命名、md 相对链接、
正文声明的配套代码路径、代码块来源标注。只用 `python3` 标准库。

## 体例约定

- **代码块逐块标注出处。** 出自配套工程的标 `// code/<场景>/<文件>（节选）`；
  示意代码标 `// 示意：…，本段不在配套仓库中`；引用标准库的标
  `// 标准库签名（摘自 <头文件>），非本仓库代码`。读者据此判断哪一段能直接拿去编译。
  ⚠️ 这条的判据与现有正文有出入，待定，见 [`AGENTS.md`](AGENTS.md) §5。
- **涉及 C++ 标准版本差异的论断，必须落成一个能被编译器检查的装置**，不能只写在正文里。
- **图**：统一用 Mermaid 内嵌，不引用外部渲染服务；需要具象画面（内存布局 / 时序 / 指针关系）
  时用手写 SVG，过程类用 GIF 动图——两者都放进该模式的 `docs/assets/`。
  同一张图的 SVG 与 GIF 尽量由**同一个脚本**产出，文案与配色共用，避免两版漂移。
- **开源案例必须回源核验**：能指出真实文件与函数名，不接受转述。

## 协作

读取本仓库的编码助手请先看 [`AGENTS.md`](AGENTS.md)；进度锚点是 [`STATE.md`](STATE.md)。

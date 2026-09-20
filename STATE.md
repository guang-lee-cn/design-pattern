---
project: design-pattern
updated: 2026-09-20T20:40:00+08:00
phase: 内容
step_id: S-007
status: done
head: 0dbd6b5
verify: python3 tools/check_pattern.py factory-pattern && cd factory-pattern/code && cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
---

## 已完成（含验证证据）

- [S-007] `factory-pattern` 改由**骨架驱动**重建，从第 1 节写起。旧的 42 个文件（三篇正文、配套工程、旧导航）整体移出仓库、归档可查。
  - 第 1 节「是什么」落成 `docs/01-工厂模式是什么.md`：伞形词拆解 + 一条判据 + 三个反例检验。
  - 三条外部出处**逐条回源核实**（GoF 23 模式与创建型 5 个 · Head First Ch.4 P.117 · Effective Java Item 1），
    出处与核实方式写进正文「出处」一节，未给出处的判断明确标注为本仓库分析。
  - 进度改按**节**记而非按篇记（见 `README.md` 的十节状态表）：篇数不再预设。
  - 证据：`check_pattern.py` **0 必须修**（1 条警告：`notes/` 为空，概念章尚未引入代码）；
    Mermaid **2/2** 渲染通过；配套工程 `cmake` + `build` 均 OK。

## 下一步（唯一）

- 动作：展开骨架第 2 节「背景与代价」。硬要求是给出**可编译的对照**（两版代码，差异可逐字节或逐输出比对），
  并建立 `code/src/02_cost/`。这是 `notes/` 出现第一条记录的预期时点。
- 触发词：`继续`

## 阻塞

- none

## 决策

- 本文件只记与**仓库结构**直接相关的结论；步骤历史只保留最近一步
- 内容层决策、素材溯源、系列排期都在仓库外，不随本仓库发布
- 判断标准：**知识进仓库，过程出仓库**
- 章节顺序服从依赖：`是什么` 必须早于 `背景与代价`
- **篇数不再预设**：按骨架的节推进，一节可以写成一篇文章，也可以拆成几篇

## 待拍板（≤1）

- none

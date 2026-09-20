---
project: design-pattern
updated: 2026-09-20T13:45:39+08:00
phase: 内容
step_id: S-008
status: active
head: 827e7c0
verify: python3 tools/check_pattern.py factory-pattern && cd factory-pattern/code && cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j
---

## 已完成（含验证证据）

- [S-008 前置] 协作换轨并归零重聊：
  - `AGENTS.md` 新增 §0（人机分工：`docs/notes` 助手落盘；`code/` 使用者手敲并亲手构建验证，助手只读不写）与 §4（人机共用同一套一键命令；测试框架/CTest/CI 等工程能力暂不展开）。
  - factory-pattern 第 1 节旧文移出工作树（可在 git 历史 `0dbd6b5` 查阅；其中三条已回源核实的出处——GoF 创建型 5 个、Head First Ch.4、Effective Java Item 1——重写时仍可复用）。
  - 十节状态全部回到 ⏸️；`code/` 仅留 CMakeLists 骨架。

## 下一步（唯一）

- 动作：**聊天阶段，不落盘**——先聊「工厂模式的诞生背景与应用场景」，为骨架第 2 节「背景与代价」、第 5 节「业务场景」积累素材。聊透且使用者明确认可后再落 `docs/`。
- 触发词：落盘由使用者明确说"写下来"。

## 阻塞

- none

## 决策

- 本文件只记与**仓库结构**直接相关的结论；步骤历史只保留最近一步
- 内容层决策、素材溯源、系列排期都在仓库外，不随本仓库发布
- 判断标准：**知识进仓库，过程出仓库**
- 章节顺序服从依赖，但**聊天顺序不限**：先聊背景与场景，落盘时再按骨架依赖归位
- **`code/` 由使用者手敲、亲手构建验证**；助手不碰 `code/`，停机前仅用同一套命令独立复核
- **篇数不再预设**：按骨架的节推进，一节可以写成一篇文章，也可以拆成几篇

## 待拍板（≤1）

- none

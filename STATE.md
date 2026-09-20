---
project: design-pattern
updated: 2026-09-20T14:13:50+08:00
phase: 内容
step_id: S-010
status: active
head: 7768372
verify: cd factory-pattern/code && sh clean.sh && sh run.sh
---

## 已完成（含验证证据）

- [S-009] 场景 1 `01_select`（运行时按条件选型 / 简单工厂）：工程收敛为两层 CMake（`src/` 用 `GLOB_RECURSE` 自动发现，target = `<节>_<场景>_old|new`）+ 工程根唯一 `run.sh [Release|Debug] [节] [场景]` + `clean.sh`；Google C++ Style；共享键名在 `include/constants.h`。
- [S-010] 场景 2 `02_framework_slot`（框架插槽 / GoF 工厂方法原生形态）：old 为框架基类 switch 直接构造具体按钮；new 为抽象 `Button` + 纯虚 `CreateButton()` 插槽，`Render()` 模板方法固定流程、子类填槽。验证（GCC 13.3.0）：零 warning；Release/Debug 下两场景 old/new 输出均逐字节一致；Debug ELF 含 debug_info；非法参数 exit=1。

## 下一步（唯一）

- 动作：罗列场景 3 `03_complex_creation`（创建昂贵/有约束——连接按逻辑名去重复用）的 `old.cpp`/`new.cpp`；使用者录入后 `sh run.sh 02_cost` 自动纳入。
- 注意：从本场景起痛点是"创建代价"，old/new 业务行为仍逐字节一致，但资源统计行预期不同（差异即收益），验证时对非资源行做 diff。
- 六个场景代码齐备并聊透、使用者说"写下来"后，助手再落第 2 节 `docs/`。

## 阻塞

- `check_pattern.py` 报 `[!!] docs/ 下没有任何 .md`：聊天/代码阶段的**预期中间态**。此阶段 `verify` 只含工程一键构建运行；docs 落盘后把 `python3 tools/check_pattern.py factory-pattern` 加回 `verify`。

## 决策

- 本文件只记与**仓库结构**直接相关的结论；步骤历史只保留最近一步
- **知识进仓库，过程出仓库**；章节顺序服从依赖，聊天顺序不限
- **`code/` 由使用者手敲、亲手构建验证**；助手不碰 `code/`，停机前仅用同一套命令独立复核
- 工程内同构重复一律用自动发现/参数消除，不写逐目录登记的转发壳
- 脚本 POSIX sh；C++17 + Google C++ Style；`#ifndef` 保护（不用 `#pragma once`）；共享键名进 `include/constants.h`
- **篇数不预设**：一节可一篇也可拆多篇

## 待拍板（≤1）

- none

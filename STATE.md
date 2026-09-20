---
project: design-pattern
updated: 2026-09-20T13:57:37+08:00
phase: 内容
step_id: S-009
status: active
head: 7a6d815
verify: cd factory-pattern/code && sh clean.sh && sh run.sh
---

## 已完成（含验证证据）

- [S-009] 第 2 节场景 1 `02_select`（多实现按条件选一个）新旧对照落地，全部代码由使用者录入：
  - 工程收敛为**两层 CMake**：根 `CMakeLists.txt`（全局设置）+ `src/CMakeLists.txt`（`GLOB_RECURSE CONFIGURE_DEPENDS` 自动发现 `src/<节>/<场景>/*.cpp`，target/可执行名 = `<节>_<场景>_old|new`，节前缀防跨节重名）；节目录与场景目录零 CMake、零脚本。
  - 工程根唯一入口 `run.sh [Release|Debug] [节] [场景]`（参数顺序任意，`--target` 精确构建）；`clean.sh` 清理；Release 进 `build/`、Debug 进 `build-debug/`。
  - Google C++ Style（PascalCase 函数、`snake_` 成员、`kCamelCase` 常量、接口无 `I` 前缀、`#ifndef` 保护）；共享键名在 `include/constants.h`。
  - 验证（GCC 13.3.0）：零 warning/error；Release 与 Debug 下 old/new 输出均逐字节一致（`diff`）；Debug ELF 含 `debug_info, not stripped`；非法节/场景参数均 `exit=1`。收敛方案曾在 `/tmp` 双节三场景探针上 10 项断言全过后才交付录入。

## 下一步（唯一）

- 动作：罗列第 2 节场景 2 `02_framework_slot`（框架插槽——GoF 工厂方法的原生形态）的 `old.cpp`/`new.cpp`；使用者录入后 `sh run.sh 02_cost` 自动纳入，无需改任何 CMake。
- 六个场景代码齐备并聊透、使用者说"写下来"后，助手再落第 2 节 `docs/`。

## 阻塞

- `check_pattern.py` 报 `[!!] docs/ 下没有任何 .md`：这是聊天/代码阶段的**预期中间态**（docs 尚未落盘）。此阶段 `verify` 只含工程一键构建运行；docs 落盘后把 `python3 tools/check_pattern.py factory-pattern` 加回 `verify`。

## 决策

- 本文件只记与**仓库结构**直接相关的结论；步骤历史只保留最近一步
- 内容层决策、素材溯源、系列排期都在仓库外，不随本仓库发布
- 判断标准：**知识进仓库，过程出仓库**
- 章节顺序服从依赖，但**聊天顺序不限**：先聊背景与场景，落盘时再按骨架依赖归位
- **`code/` 由使用者手敲、亲手构建验证**；助手不碰 `code/`，停机前仅用同一套命令独立复核
- 工程内同构重复一律用自动发现/参数消除，不写逐目录登记的转发壳（与场景 6 注册表同一思想）
- **篇数不再预设**：按骨架的节推进，一节可以写成一篇文章，也可以拆成几篇

## 待拍板（≤1）

- none

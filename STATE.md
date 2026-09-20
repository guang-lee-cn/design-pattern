---
project: design-pattern
updated: 2026-09-20T14:30:00+08:00
phase: 基础设施
step_id: S-004
status: done
head: 07dc15e
verify: cd factory-pattern/code && cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ctest --test-dir build --output-on-failure
---

## 已完成（含验证证据）

- [S-004] 仓库重定位为「可公开的设计模式实践库」——
  - **目录改为一模式一自包含文件夹**：配套工程由仓库根迁入 `factory-pattern/code/`
  - **流程文件移出仓库**：决策记录、迭代队列、旧技能副本移到仓库外，不再随远端发布
  - **协作协议退化为平台无关的最小锚点**：`AGENTS.md` + `tools/check_state.py`，只依赖 `git` 与 `python3` 标准库
  - **脱敏**：身份信息、平台专名、指向仓库外的绝对路径全部清除
  - 证据：残留扫描 **0** 命中；`cmake --build` 零 warning；`ctest` **2/2** 通过；
    正文中 6 处工程路径声明逐条指向真实目录；仓库内 markdown 链接 0 失效

## 下一步（唯一）

- 动作：写第 2 篇《简单工厂、工厂方法、抽象工厂：不是三个并列模式》的代码骨架与引言节
- 触发词：`继续`

## 阻塞

- none

## 决策

- 本文件只记与**仓库结构**直接相关的结论
- 内容层决策（讲什么、怎么讲）在该模式的 `plan/` 与 `notes/` 里
- 协作流程本身不进仓库：仓库只承载设计模式（知识拆解、代码、问题复盘）

## 待拍板（≤1）

- none

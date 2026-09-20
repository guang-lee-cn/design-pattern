---
project: design-pattern
updated: 2026-09-20T11:30:00+08:00
phase: P0 基础设施
step_id: S-002
status: done
head: 36b6e51
verify: cd code && cmake -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build -j && ctest --test-dir build --output-on-failure
---

## 已完成（含验证证据）

- [S-000] 工作区从 `D:\liguang\system_study\design-pattern` 迁至 WSL 并重排目录 ——
  `find . -type f -not -path './.git/*' | wc -l` → 54 个文件（`code/build/` 的 90 个编译产物已排除）
- [S-001] 接入 small-step-loop 协议并修订为 v2（结构边界取代数字预算）——
  `check_state.py` → EXIT=0
- [S-002] **修正 §7.2 隐式移动的版本错误**（迁移验收时编译器抓出来的）——
  `cmake --build` + `ctest` → EXIT=0、**零 warning**、2/2 通过；
  `probe_unique_ptr` 打印 `__cplusplus=201703` 自证走 C++17 分支；
  mermaid 渲染 4/4 OK；代码来源核对 19 块、0 块需补标注

## 下一步（唯一）

- 动作：建 `code/src/02_three_factories/` 代码骨架与"简单工厂"版，**先给变更清单待确认**
- 触发词：`继续`

## 阻塞

- none

## 决策

- D-006 工作目录迁 WSL，git 管理流推进（李广 2026-09-20）
- D-007 `factory-pattern/` = 文章与计划，`code/` = 配套代码（目录名冲突的解法）
- D-008 迭代单元粒度 = 内容结构上的"节"，**不设行数上限**（李广 2026-09-20）
- D-009 技能副本纳入本仓库；好用后再进 `~/code/skills/`（李广 2026-09-20）
- D-011 第 7 篇的 6 个开源项目必须逐条回源核验（Fast DDS 已核 1/6，发现转述错误）
- D-012 **代码"写完了"不等于"写对了"**：第 4 篇 §7.2 的错误证明未编译过的代码不可信。
  凡涉及语言规则的论断，必须落到一个能被编译器检查的装置上

## 待拍板（≤1）

- 每轮的 `verify` 由我跑还是仍按原分工由你跑？**默认我跑**（WSL 里编译是秒级，且不跑就无法声明 done）。一句话即可推翻。

# 迭代队列 —— design-pattern /《C++ 工厂模式》系列

> 一行一个**可独立提交**的单元，粒度 = 内容结构上的一个"节"（见 `skills/small-step-loop/SKILL.md` §2）。
> 顺序即依赖顺序。**不按行数卡量**；拆分的理由必须是"结构上独立"，不是"数字上超了"。
> 状态取值：`pending` / `active` / `done`。**`active` 不允许跨会话**。

## 已完成

| id | 意图 | 结构范围 | 验证命令 | 依赖 | 状态 | commit |
|---|---|---|---|---|---|---|
| S-000 | 工作区从 `D:\liguang` 迁至 WSL 并重排目录 | `design-pattern/**` | `find . -type f -not -path './.git/*' \| wc -l` → 54 | — | done | 首次提交一并定格 |
| S-001 | 接入 small-step-loop 协议（v2） | `STATE.md` `ITERATION_PLAN.md` `DECISIONS.md` `.gitignore` `README.md` | `python3 skills/small-step-loop/scripts/check_state.py .` | S-000 | done | 首次提交一并定格 |
| S-002 | 修正 §7.2 隐式移动的版本错误（迁移验收时编译器抓出） | `code/src/04_modern_cpp17/probe_unique_ptr.cpp` + `posts/04-*.md` | `cd code && cmake --build build -j && ctest --test-dir build` | S-001 | done | 见 git log |
| S-003 | 技能 v2.1：初始化单元的 verify 必须含真实构建 | `skills/small-step-loop/SKILL.md` | `diff -r` 真身与副本 | S-002 | done | 见 git log |

> S-000 / S-001 是 git 初始化前的产物，其 commit 列在首次提交里一并定格，不单独列 hash。

## P1 — 第 2 篇《简单工厂、工厂方法、抽象工厂：不是三个并列模式》

| id | 意图 | 结构范围 | 验证命令 | 依赖 | 状态 | commit |
|---|---|---|---|---|---|---|
| S-010 | 建第 2 篇代码骨架 + "简单工厂"版（可编译） | `code/src/02_three_factories/` | `cmake --build code/build -j && ./code/build/src/02_three_factories/fp_02_main` | S-001 | pending | |
| S-011 | 引言节：GoF 只承认两个模式 | `posts/02-*.md` §引言 | `python3 ~/.workbuddy/skills/md-mermaid-deliverable/scripts/check_mermaid.py posts/02-*.md` | S-010 | pending | |
| S-012 | 节：简单工厂是 idiom，不是模式 | §2.1 + 对应代码 | 同上 | S-011 | pending | |
| S-013 | 节：工厂方法把"选哪个"下推给子类 | §2.2 + 对应代码 | 同上 | S-012 | pending | |
| S-014 | 节：抽象工厂的本质是**产品族配套约束** | §2.3 + 对应代码 | 同上 | S-013 | pending | |
| S-015 | 节：三者是一条演化线上的三个点 | §2.4 + 演化图 | 同上 | S-014 | pending | |
| S-016 | 节：对比表补"何时选哪个"列，并入 §6.4 四方案取舍 | §2.5 | 同上 | S-015 | pending | |
| S-017 | 节：小结；全篇收口（图渲染 + 代码来源标注核对） | §2.6 + 全篇 | 两条脚本均 `EXIT=0` | S-016 | pending | |

## 后续篇 —— **里程碑占位**

> ⚠️ 下面每一行都是**里程碑，不是可执行单元**。开写前必须按 `SKILL.md` §2.3 的四条件拆成节级单元，
> 不得把一整篇当一步做。拆完把本表这几行替换掉。

| id | 里程碑 | 结构范围 | 验证命令 | 依赖 | 状态 | commit |
|---|---|---|---|---|---|---|
| S-020 | 第 3 篇《抽象工厂实战：产品族、组合根与自注册》 | `posts/03-*.md` + `code/src/03_*/` | 待拆 | S-017 | pending | |
| S-030 | 第 6 篇《什么时候不该用工厂：过度设计的信号》 | `posts/06-*.md` | 待拆 | S-020 | pending | |
| S-040 | **第 7 篇的前置**：§9 其余 5 个开源项目源码核验 | `refs/verification/*.md` | 每个项目至少 1 条源码级证据（`grep` 输出或永久链接） | S-001 | pending | |
| S-041 | 第 7 篇《从 Fast DDS 到 Nginx：开源项目里的工厂模式》 | `posts/07-*.md` | 待拆 | S-030, S-040 | pending | |
| S-050 | 第 8 篇：系列索引（1 屏） | `posts/08-*.md` | 全部链接可达 | S-041 | pending | |

## 拆步记录

> 每次拆步留一行原因。**若原因多为"数字超了"，说明切分点找错了**，回 `SKILL.md` §2.3 重新定节。

| 原单元 | 拆成 | 原因 |
|---|---|---|
| "迁移 + 协议接入"（原计划一步） | S-000 / S-001 | 结构上独立：迁移是文件系统操作（且必须先于 `git init`），协议接入是新增元文件 |
| S-040（原并入第 7 篇） | S-040 / S-041 | 回源核验是**独立可验证**的调研产物，与写作不同源；且核验结论可能推翻文章结构 |
| —（计划外插入） | S-002（新增单元） | 迁移验收时编译器报错，暴露第 4 篇代码**从未被编译过**。根因已定位，按 §3.2 硬条件 4 立即修复。**教训：迁移必须带编译验证** |

## 非目标（本阶段明确不做）

- **不重写已完成的第 1 / 4 / 5 篇**（除非发现事实错误）。
- **不回溯重编译**已交付的三篇配套代码。
- **不迁、不改 `factory-pattern-doubao/`**（对照方产出，只读）。
- **不改 `refs/` 下的源素材**（仅作溯源；与正文冲突时以正文为准，并把冲突登记进 `DECISIONS.md`）。

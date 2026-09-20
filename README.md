# design-pattern

C++ 设计模式系列文章的写作与配套代码工作区。

## 布局

| 路径 | 内容 |
|---|---|
| `factory-pattern/` | 《C++ 工厂模式》系列：文章、计划、路线图 |
| `factory-pattern/posts/` | 正式正文（markdown + 内嵌 Mermaid） |
| `factory-pattern/plan/` | 章节计划、系列路线图 |
| `factory-pattern/archive/` | 历史形态产物（docx / html），仅作参考 |
| `code/` | 配套 C++ 代码仓库（CMake，逐篇增量增长） |
| `refs/` | 源素材与评审记录（**不改写**，仅作溯源） |
| `skills/small-step-loop/` | 协同工作协议副本（真身在 `~/.workbuddy/skills/`） |

## 协作方式

本项目按 `skills/small-step-loop/` 协议推进：**小步迭代、间奏可停、断点快启**。

- **恢复入口**：本目录的 `STATE.md`（唯一）。新会话第一步读它。
- **队列**：`ITERATION_PLAN.md`，一行一个可独立提交的单元，粒度 = 内容结构上的一个"节"。
- **决策**：`DECISIONS.md`，已定型的事不再重复讨论。
- **git**：一迭代一提交；同步 `STATE.md` 的提交用 `chore(state):` 前缀。

```bash
# 恢复：对齐状态，不产出内容
cat STATE.md
python3 skills/small-step-loop/scripts/check_state.py . --resume
```

## 代码

```bash
cd code
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
ctest --test-dir build --output-on-failure
```

要求 C++17 及以上（第 4 篇用到 `std::optional` / `if constexpr` / 自注册装置）。
`src/CMakeLists.txt` 按子目录存在性自动纳入，**新增篇章无需改动已有文件**。

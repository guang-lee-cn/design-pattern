# tools/ —— 工具与依赖登记

跨场景复用的工具住这里：仓库治理门禁（结构与状态校验），以及被 `docs/` 引为「可复现证据」的
图生成脚本。**场景绑定的构建入口**（`build.sh` / `Makefile` / `run.sh`）留在各自场景目录内
—— 见 [`AGENTS.md`](../AGENTS.md) §4。

## 单一来源

工具实体只存在于本仓库，随提交演进；skill 与外部流程**调用**本目录，不复制副本。
这样工具的每一处改动都落在 git 历史里 —— 可溯源、可回退、可对照。

> **2026-09-30 拍板**：此前 `TEMPLATE.md` §6 把「技能、协作脚本」列为**不进仓库**，
> 结果是同一判据在仓库内外各有一份实现、改一处另一处不知道（`tools/check_pattern.py`
> 的「代码块来源标注」与仓库外 skill 的 `check_code_provenance.py` 就是一对）。
> 该条已推翻：**工具脚本进仓库**，只有「特定助手平台的专名」不进（见该表现行条文）。

## 非系统默认依赖

仓库本体只要求 **C++17 编译器 + `python3` 标准库**就能跑完全部门禁。
下表这些只有**重新生成图**或**取覆盖 / 符号数据**时才需要。

| 依赖 | 用途 | 谁需要 | 安装（Debian / Ubuntu） | 自检 |
|---|---|---|---|---|
| `rsvg-convert` | SVG → PNG 栅格化（GIF 逐帧合成的上一步） | `06_cost` / `07_contract` / `A06_plugin` 的图生成脚本 | `apt install librsvg2-bin` | `rsvg-convert --version` |
| `Pillow` | PNG 逐帧合成 GIF | 同上（三个 `make_*.py`） | `apt install python3-pil` | `python3 -c "import PIL;print(PIL.__version__)"` |
| `gcov` | 分支覆盖计数 | `01_birth`（可测试性的量化证据） | 随 `gcc` | `gcov --version` |
| `nm` | 符号表检查（ABI 实验） | `A06_plugin` | 随 `binutils` | `nm --version` |

**图已随仓库提交**（`docs/assets/`）。不复现图的人**不必装** `rsvg-convert` / `Pillow`
—— 这一条是刻意的：读文档的人与重构图的人是两类人，不让前者为后者的依赖买单。

## 工具清单

### 治理门禁（纯标准库，任何环境可跑）

| 脚本 | 作用 |
|---|---|
| [`check_pattern.py`](check_pattern.py) | 一个模式是否符合 [`TEMPLATE.md`](../TEMPLATE.md)：结构 / 三层命名 / 相对链接 / 配套路径 / 来源标注 |
| [`check_state.py`](check_state.py) | [`STATE.md`](../STATE.md) 状态锚点是否自洽 |

两个脚本都只做**可机械判定**的检查，判不了的留给人工（见 `AGENTS.md` §7）。

### 场景内工具（随场景提交，不跨场景复用）

路径相对 `factory-pattern/code/`。

| 脚本 | 场景 | 作用 | 额外依赖 |
|---|---|---|---|
| `build.sh` | `01_birth` | 三痛点 + 可测试性对照 | `gcov` |
| `multi_tu/run.sh` + `Makefile` | `03_simple_factory` | 多 TU 重建范围实测 | — |
| `measure_extend_cost.py` | `05_abstract_factory` | 家族扩展代价（用编译器报错数当量化指标） | — |
| `count_loc.sh` | `06_cost` | 有效代码行统计（剔除注释与空行） | — |
| `make_06_dispatch.py` | `06_cost` | 分派链图：静态 SVG + GIF 动图 | `rsvg-convert` `Pillow` |
| `build.sh` | `07_contract` | 所有权 / 失败语义实验 | `-fsanitize=address` |
| `make_07_uaf.py` | `07_contract` | 悬垂指针时序：**一脚本出 SVG + GIF 两形态** | `rsvg-convert` `Pillow` |
| `build.sh` | `A04_c` | C 语言两种链接方式对照 | — |
| `build.sh` + `04_gen_modules.sh` | `A05_nginx_redis` | 构建期代码生成（nginx `auto/modules` 的做法） | — |
| `build.sh` | `A06_plugin` | `dlopen` 五组实验 | `nm` |
| `make_a06_abi.py` | `A06_plugin` | ABI 字段错位逐格扫描图 | `rsvg-convert` `Pillow` |

> **为什么场景内工具不集中到本目录**：`AGENTS.md` §4 要求每个场景能单独拷走、单独跑，
> 把 `build.sh` 挪出来会破坏这条。已知代价是三个 `make_*.py` 有 **8 项同形样板**
> （含逐字节相同的 `CJK_FONT` 字面量 ×3、`HERE/ROOT/ASSETS` 路径推导 ×3、`SCALE = 2` ×3）。
> **裁决：登记为已知代价，不抽共享模块** —— 三个脚本各自服务不同的图，改动频率低；
> 而跨目录 import 会让「单独拷走」直接断掉，比重复更脆。

## 与仓库外 skill 的分工

| 工具的适用面 | 住哪 | 理由 |
|---|---|---|
| 只对**本仓库**有意义（校验 `TEMPLATE.md` / `STATE.md` / 本仓库文档） | 本目录 | 离开仓库无意义 |
| 对**任何 markdown 交付**都有意义（渲染兼容性、mermaid 语法） | 仓库外 skill | 跨项目复用，且含平台专名 |

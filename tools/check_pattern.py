#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_pattern.py —— 按 TEMPLATE.md 复核一个模式文件夹。

用法：
    python3 tools/check_pattern.py factory-pattern
    python3 tools/check_pattern.py factory-pattern --repo-root .
    python3 tools/check_pattern.py factory-pattern --quiet

只依赖 Python 3 标准库，不需要 git、不需要网络。
输出 [!!] 为必须修，[!] 为警告。退出码 1 表示存在 [!!]。

2026-09-30 修订（四条规则曾经严重过时，对工厂模式报出 43 条假失败）：
  1. docs/ 命名接受**三层**（`NN-` / `NNx-` 子编号 / `A0N-` 专题 / `quizNN-` 测验），不再只认 `\\d{2}-`；
  2. `src/` `include/` 开头的路径视为**上游源码引用**，不要求落在 `code/` 下；
     本仓库配套代码一律以 `code/` 前缀引用，必须真实存在；
  3. md 相对链接检查先剥掉行内代码段——`` `![](assets/x.svg)` `` 是引用语法示例，不是真链接；
  4. `code/` 的编译入口放宽为「存在任一构建脚本」，不再要求顶层 `CMakeLists.txt`。

2026-09-30 再修订（另两条规则与正文实际体例相反，对工厂模式报出 85 条假警告）：
  5. 顶层节中文序号降为**可选**体例 —— 全库 17 篇里只有 06 / 07 用；整篇不用不报，
     且 `## 进度`（体例定义的尾部状态块）不计入正文节。
  6. 代码块「来源」只指**引用他人代码**的出处（公开论坛 / 仓库）。本文档自己写的
     教学示例没有来源可标，不再告警；只在块**自述引用**时要求出处可定位。
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

# 三层命名（与 docs/00-讲解计划.md 的定义一致）：
#   主线  `NN-<标题>.md`，子编号写成紧贴后缀 `NNx-`（如 `04a-` / `04b-`）
#   专题  `A0N-<标题>.md`
#   测验  `quizNN-<标题>.md`
DOC_MAIN_RE = re.compile(r"^(\d{2})([a-z]?)-(.+)\.md$")
DOC_TOPIC_RE = re.compile(r"^A(\d{2})-(.+)\.md$")
DOC_QUIZ_RE = re.compile(r"^quiz(\d{2})-(.+)\.md$")
NOTE_RE = re.compile(r"^(\d{3})-(.+)\.md$")
LINK_RE = re.compile(r"\[[^\]\n]*\]\(([^)\s]+)\)")
# 本仓库配套代码：正文一律写成 `code/...`（相对**模式根**），必须真实存在。
LOCAL_CODE_RE = re.compile(
    r"(?:^|[\s`'\"])(code/[A-Za-z0-9_][A-Za-z0-9_./+-]*"
    r"\.(?:cpp|cc|cxx|c|h|hpp|sh|py|txt|cmake))"
)
# 上游源码引用：`src/...`、`include/...`。**本仓库 code/ 下不存在这两个顶层目录**，
# 所以这类路径一律视为第三方源码引用——考据类文档（A02~A05）的正文主体就是引用上游代码，
# 只计数、不要求落在 code/ 下。
UPSTREAM_PATH_RE = re.compile(
    r"(?:^|[\s`'\"])((?:src|include)/[A-Za-z0-9_./+-]*\.(?:cpp|cc|cxx|h|hpp|txt|cmake|json))"
)
# 构建入口：code/ 下出现任意一个即视为示例可独立编译
BUILD_ENTRY_NAMES = ("CMakeLists.txt", "Makefile", "build.sh", "run.sh")
# 「来源」= **公开论坛 / 仓库中引用的他人代码**的出处。本仓库正文里的代码块绝大多数是
# 教学示例（文档自己写的），没有来源可标 —— 所以「无来源标注」不是缺陷。
# 只有**自述引用了他人代码**的块才要求出处，判定词如下（实测正文 76 个块里命中 0 个）：
CITE_MARKERS = ("摘自", "来源", "截取", "改编自", "参考自", "复制自", "源自",
                "不在配套仓库中", "非本仓库代码", "仓库里是")
# 出处的「可定位」形式：本仓库路径 / 上游路径 / URL / 带扩展名的文件名
# （`// logger.h`、`// PersistenceFactory.cpp L46` 这种写法即命中最后一条）。
PROVENANCE_FILE_RE = re.compile(r"\.(?:cpp|cc|cxx|c|h|hpp|hh|py|sh|json|txt|cmake)\b")
# 需要检查来源标注的代码块语言
CODE_LANGS = {"cpp", "c", "cc", "cxx", "c++", "h", "hpp", "cmake", "cmake-literal"}
# 本仓库不允许出现的遗留形态
LEGACY_SUFFIXES = {".docx", ".doc", ".html", ".htm", ".psd"}

# 参与不了序号统计的固定块（篇内状态栏，由 00-讲解计划.md 的体例定义）
NON_BODY_HEADS = ("进度",)

CN_DIGITS = {"一": 1, "二": 2, "三": 3, "四": 4, "五": 5,
             "六": 6, "七": 7, "八": 8, "九": 9, "十": 10}


def doc_key(name: str):
    """把 docs/ 下的文件名归成 `层:序号` 键；不合三层体例返回 None。

    子编号（`04a-` / `04b-`）与主号（`04-`）算不同键，所以它们并列不报「序号重复」。
    """
    m = DOC_MAIN_RE.match(name)
    if m:
        return "主线:" + m.group(1) + m.group(2)
    m = DOC_TOPIC_RE.match(name)
    if m:
        return "专题:A" + m.group(1)
    m = DOC_QUIZ_RE.match(name)
    if m:
        return "测验:quiz" + m.group(1)
    return None


def cn_ordinal(text: str):
    """把 '一' / '十' / '十一' / '二十一' 转成整数；认不出返回 None。"""
    text = text.strip()
    if not text:
        return None
    if text == "十":
        return 10
    if text.startswith("十"):
        rest = text[1:]
        return 10 + CN_DIGITS.get(rest, 0) if len(rest) == 1 else None
    if "十" in text:
        head, _, tail = text.partition("十")
        if len(head) != 1 or head not in CN_DIGITS:
            return None
        if tail == "":
            return CN_DIGITS[head] * 10
        if len(tail) == 1 and tail in CN_DIGITS:
            return CN_DIGITS[head] * 10 + CN_DIGITS[tail]
        return None
    if len(text) == 1:
        return CN_DIGITS.get(text)
    return None


def build_entries(code: Path):
    """列出 code/ 下的构建入口，跳过 build/ 等产物目录。"""
    out = []
    for p in sorted(code.rglob("*")):
        if not p.is_file() or p.name not in BUILD_ENTRY_NAMES:
            continue
        parents = p.relative_to(code).parts[:-1]
        if any(d == "build" or d.startswith("build-") or d == ".git"
               for d in parents):
            continue
        out.append(p)
    return out


def fenced_blocks(lines):
    """产出 (起始行号, 语言, 内容行列表)。行号从 1 起。"""
    blocks = []
    start = None
    lang = ""
    body = []
    for i, line in enumerate(lines, 1):
        stripped = line.lstrip()
        if stripped.startswith("```"):
            if start is None:
                start = i
                lang = stripped.strip("`").strip().lower()
                body = []
            else:
                blocks.append((start, lang, body))
                start = None
            continue
        if start is not None:
            body.append(line)
    if start is not None:  # 未闭合
        blocks.append((start, lang, body))
    return blocks


def header_comment(body, limit=6):
    """取代码块开头连续的注释行（`//` 或 `#`），最多 limit 行。"""
    out = []
    for line in body[:limit]:
        s = line.strip()
        if not s:
            if out:
                break
            continue
        if s.startswith("//") or s.startswith("#"):
            out.append(s)
            continue
        break
    return out


class Report:
    def __init__(self):
        self.items = []

    def fail(self, msg):
        self.items.append(("!!", msg))

    def warn(self, msg):
        self.items.append(("!", msg))

    def ok(self, msg):
        self.items.append(("ok", msg))

    def count(self, level):
        return sum(1 for lvl, _ in self.items if lvl == level)


def check_pattern(root: Path, quiet=False) -> int:
    rep = Report()
    pat = root

    if not pat.is_dir():
        rep.fail(f"模式目录不存在：{pat}")
        emit(rep, quiet)
        return 1

    name = pat.name
    print(f"复核：{pat}")
    print()

    # ---- 1. 必需文件与目录 ----
    if not (pat / "README.md").is_file():
        rep.fail("缺 README.md —— 该模式的导航入口")
    else:
        rep.ok("README.md 存在")

    docs = pat / "docs"
    if not docs.is_dir():
        rep.fail("缺 docs/ —— 正文目录")
    notes = pat / "notes"
    if not notes.is_dir():
        rep.warn("缺 notes/ —— 问题记录与复盘目录（真踩过坑才写得出来，写到再建）")
    code = pat / "code"
    if not code.is_dir():
        rep.fail("缺 code/ —— 配套工程目录")
    else:
        entries = build_entries(code)
        if not entries:
            rep.fail("code/ 下找不到任何构建入口"
                     "（CMakeLists.txt / Makefile / build.sh / run.sh）—— 示例无法独立编译")
        else:
            shown = "、".join(str(p.relative_to(code)) for p in entries[:3])
            more = f"，另 {len(entries) - 3} 个" if len(entries) > 3 else ""
            rep.ok(f"code/ 构建入口 {len(entries)} 个（{shown}{more}）")

    doc_files = []
    if docs.is_dir():
        for p in sorted(docs.iterdir()):
            if p.is_file() and p.suffix == ".md":
                doc_files.append(p)
        if not doc_files:
            rep.fail("docs/ 下没有任何 .md")
        else:
            keys = {}
            for p in doc_files:
                key = doc_key(p.name)
                if key is None:
                    rep.fail(f"docs/{p.name} 命名不合三层体例："
                             "`NN-<标题>.md` / `NNx-<标题>.md` / `A0N-<标题>.md` / `quizNN-<标题>.md`")
                    continue
                keys.setdefault(key, []).append(p.name)
            for key, names in sorted(keys.items()):
                if len(names) > 1:
                    rep.fail(f"docs/ 序号 {key} 重复：{', '.join(names)}")
            if not any(lvl == "!!" for lvl, m in rep.items if "docs/" in m):
                layers = sorted({k.split(":", 1)[0] for k in keys})
                rep.ok(f"docs/ 下 {len(doc_files)} 篇，三层命名合规、序号无重复"
                       f"（{'/'.join(layers)}）")

    note_files = []
    if notes.is_dir():
        for p in sorted(notes.iterdir()):
            if p.is_file() and p.suffix == ".md":
                note_files.append(p)
        if not note_files:
            rep.warn("notes/ 是空的 —— 一个从没踩过坑的模式，通常是还没真跑过代码")
        else:
            nums = {}
            for p in note_files:
                m = NOTE_RE.match(p.name)
                if not m:
                    rep.fail(f"notes/{p.name} 命名不合 `NNN-<问题>.md`")
                    continue
                nums.setdefault(m.group(1), []).append(p.name)
            for num, names in sorted(nums.items()):
                if len(names) > 1:
                    rep.fail(f"notes/ 序号 {num} 重复：{', '.join(names)}")

    # ---- 2. 遗留形态 ----
    legacy = [p.relative_to(pat) for p in pat.rglob("*")
              if p.is_file() and p.suffix.lower() in LEGACY_SUFFIXES
              and ".git" not in p.parts and "build" not in p.parts]
    if legacy:
        for p in legacy[:10]:
            rep.warn(f"遗留形态文件：{p} —— 本仓库只用 markdown + 内嵌 Mermaid")
    else:
        rep.ok("无 .docx/.html 等遗留形态")

    # ---- 3. md 相对链接 ----
    md_files = [pat / "README.md"] + doc_files + note_files
    broken = []
    checked_links = 0
    for p in md_files:
        if not p.is_file():
            continue
        for lineno, line in enumerate(p.read_text(encoding="utf-8").splitlines(), 1):
            # 先剥掉行内代码段：写在反引号里的 `[](assets/x.svg)` 是**引用语法示例**，不是真链接
            for target in LINK_RE.findall(re.sub(r"`[^`]*`", "", line)):
                if target.startswith(("http://", "https://", "mailto:", "#")):
                    continue
                checked_links += 1
                rel = target.split("#", 1)[0].strip()
                if not rel:
                    continue
                dest = (p.parent / rel).resolve()
                if not dest.exists():
                    broken.append(f"{p.relative_to(pat)}:{lineno} → {target}")
    if broken:
        for b in broken[:15]:
            rep.fail(f"链接失效：{b}")
        if len(broken) > 15:
            rep.fail(f"…另有 {len(broken) - 15} 条失效链接")
    else:
        rep.ok(f"md 相对链接 {checked_links} 条，全部可达")

    # ---- 4. 正文声明的配套代码路径 ----
    # `code/...`（相对模式根）= 本仓库配套代码，必须存在；
    # `src/...` `include/...` = 上游第三方源码引用，只计数、不要求存在。
    missing = []
    declared = 0
    upstream = 0
    for p in doc_files:
        for lineno, line in enumerate(p.read_text(encoding="utf-8").splitlines(), 1):
            scan = re.sub(r"https?://\S+", "", line)   # 去掉 URL，免得把网址里的 code/ 当本地路径
            for rel in LOCAL_CODE_RE.findall(scan):
                declared += 1
                if not (pat / rel).exists():
                    missing.append(f"{p.name}:{lineno} → {rel}")
            upstream += len(UPSTREAM_PATH_RE.findall(scan))
    if missing:
        for m in sorted(set(missing)):
            rep.fail(f"正文声明的配套代码路径不存在：{m}")
    elif declared:
        rep.ok(f"正文声明的配套代码路径 {declared} 处，全部存在")
    if upstream:
        rep.ok(f"上游源码引用 {upstream} 处（考据类引用，不要求在 code/ 下）")

    # ---- 5. 顶层节 ----
    # 中文序号是**可选**体例：全库 17 篇里只有 06 / 07 用（各 6 / 7 节），其余 15 篇不用。
    # 所以「整篇不用序号」不报；只在**用了**的时候要求连续。
    # `## 进度` 是 00-讲解计划.md 定义的篇内状态块，不是正文节，不参与序号统计。
    for p in doc_files:
        text = p.read_text(encoding="utf-8")
        heads = [h for h in re.findall(r"^## (.+)$", text, re.M)
                 if h.strip().rstrip("：:") not in NON_BODY_HEADS]
        if len(heads) < 2:
            rep.fail(f"{p.name} 顶层节只有 {len(heads)} 个 —— 不足两节不成文章")
            continue
        ordinals = []
        plain = []
        for h in heads:
            head = h.split("、", 1)[0].strip()
            val = cn_ordinal(head)
            if val is None:
                plain.append(h)
            else:
                ordinals.append(val)
        if not ordinals:
            continue                      # 整篇不用序号 —— 合法体例
        if plain:
            rep.warn(f"{p.name} 顶层节混用序号：{len(plain)} 节无序号"
                     f"（{'；'.join(plain[:3])}）")
        if ordinals != list(range(ordinals[0], ordinals[0] + len(ordinals))):
            rep.warn(f"{p.name} 顶层节序号不连续：{ordinals}")

    # ---- 6. 代码块来源 ----
    # 「来源」= 公开论坛 / 仓库中**引用他人代码**时的出处（2026-09-30 裁决）。
    # 正文里的块绝大多数是教学示例，由文档自己写，没有来源可标 —— 不再告警。
    # 保留的唯一约束：**自述引用了他人代码**的块，出处必须可定位。
    n_orig = 0
    n_cited = 0
    unlocatable = []
    for p in doc_files:
        lines = p.read_text(encoding="utf-8").splitlines()
        for start, lang, body in fenced_blocks(lines):
            if lang not in CODE_LANGS:
                continue
            head = " ".join(header_comment(body))
            if not any(m in head for m in CITE_MARKERS):
                n_orig += 1
                continue
            n_cited += 1
            locatable = bool(LOCAL_CODE_RE.search(head)
                             or UPSTREAM_PATH_RE.search(head)
                             or re.search(r"https?://\S+", head)
                             or PROVENANCE_FILE_RE.search(head))
            if not locatable:
                first = next((l.strip() for l in body if l.strip()), "")
                unlocatable.append(f"{p.name}:{start} [{lang}] {first[:70]}")
    for u in unlocatable:
        rep.warn(f"自述引用了他人的代码，但出处不可定位：{u}")
    if n_orig + n_cited:
        tail = "出处均已定位" if not unlocatable else f"其中 {len(unlocatable)} 个出处不可定位"
        rep.ok(f"代码块 {n_orig + n_cited} 个：原创示例 {n_orig} 个、"
               f"声明引用他人代码 {n_cited} 个（{tail}）")

    emit(rep, quiet)
    return 1 if rep.count("!!") else 0


def emit(rep: Report, quiet: bool):
    print("—— 复核结果 ——")
    order = {"!!": 0, "!": 1, "ok": 2}
    for level, msg in sorted(rep.items, key=lambda x: order[x[0]]):
        if level == "!!":
            print(f"[!!] {msg}")
        elif level == "!":
            print(f"[!]  {msg}")
        elif not quiet:
            print(f"[ok] {msg}")
    print()
    print(f"必须修 {rep.count('!!')} 条，警告 {rep.count('!')} 条")


def main() -> int:
    ap = argparse.ArgumentParser(description="按 TEMPLATE.md 复核一个模式文件夹")
    ap.add_argument("pattern", help="模式目录，如 factory-pattern")
    ap.add_argument("--repo-root", default=None,
                    help="仓库根，默认取脚本所在目录的上一级")
    ap.add_argument("--quiet", action="store_true", help="不打印 [ok] 项")
    args = ap.parse_args()

    if args.repo_root:
        root = Path(args.repo_root).resolve()
    else:
        root = Path(__file__).resolve().parent.parent
    target = Path(args.pattern)
    if not target.is_absolute():
        target = root / target
    return check_pattern(target, quiet=args.quiet)


if __name__ == "__main__":
    sys.exit(main())

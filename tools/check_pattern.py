#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_pattern.py —— 按 TEMPLATE.md 复核一个模式文件夹。

用法：
    python3 tools/check_pattern.py factory-pattern
    python3 tools/check_pattern.py factory-pattern --repo-root .
    python3 tools/check_pattern.py factory-pattern --quiet

只依赖 Python 3 标准库，不需要 git、不需要网络。
输出 [!!] 为必须修，[!] 为警告。退出码 1 表示存在 [!!]。
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

DOC_RE = re.compile(r"^(\d{2})-(.+)\.md$")
NOTE_RE = re.compile(r"^(\d{3})-(.+)\.md$")
LINK_RE = re.compile(r"\[[^\]\n]*\]\(([^)\s]+)\)")
# `src/...`、`include/...` 形式，带源码类扩展名
SRC_PATH_RE = re.compile(
    r"(?:^|[\s/`'\"])((?:src|include)/[A-Za-z0-9_./+-]*\.(?:cpp|cc|cxx|h|hpp|txt|cmake|json))"
)
# 代码块首部注释里出现任意一个，即视为已标注来源
PROVENANCE_MARKERS = ("src/", "include/", "不在配套仓库中", "非本仓库代码", "仓库里是")
# 需要检查来源标注的代码块语言
CODE_LANGS = {"cpp", "c", "cc", "cxx", "c++", "h", "hpp", "cmake", "cmake-literal"}
# 本仓库不允许出现的遗留形态
LEGACY_SUFFIXES = {".docx", ".doc", ".html", ".htm", ".psd"}

CN_DIGITS = {"一": 1, "二": 2, "三": 3, "四": 4, "五": 5,
             "六": 6, "七": 7, "八": 8, "九": 9, "十": 10}


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
        rep.fail("缺 notes/ —— 问题记录与复盘目录")
    code = pat / "code"
    if not code.is_dir():
        rep.fail("缺 code/ —— 配套工程目录")
    elif not (code / "CMakeLists.txt").is_file():
        rep.fail("code/CMakeLists.txt 不存在 —— 工程无法独立编译")
    else:
        rep.ok("code/CMakeLists.txt 存在")

    doc_files = []
    if docs.is_dir():
        for p in sorted(docs.iterdir()):
            if p.is_file() and p.suffix == ".md":
                doc_files.append(p)
        if not doc_files:
            rep.fail("docs/ 下没有任何 .md")
        else:
            nums = {}
            for p in doc_files:
                m = DOC_RE.match(p.name)
                if not m:
                    rep.fail(f"docs/{p.name} 命名不合 `NN-<标题>.md`")
                    continue
                nums.setdefault(m.group(1), []).append(p.name)
            for num, names in sorted(nums.items()):
                if len(names) > 1:
                    rep.fail(f"docs/ 序号 {num} 重复：{', '.join(names)}")
            if not any(lvl == "!!" for lvl, m in rep.items if "docs/" in m):
                rep.ok(f"docs/ 下 {len(doc_files)} 篇，序号无重复")

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
            for target in LINK_RE.findall(line):
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
    missing = []
    declared = 0
    if code.is_dir():
        for p in doc_files:
            for lineno, line in enumerate(p.read_text(encoding="utf-8").splitlines(), 1):
                for rel in SRC_PATH_RE.findall(line):
                    declared += 1
                    if not (code / rel).exists():
                        missing.append(f"{p.name}:{lineno} → {rel}")
    if missing:
        for m in sorted(set(missing)):
            rep.fail(f"正文声明的代码路径不存在于 code/：{m}")
    elif declared:
        rep.ok(f"正文声明的代码路径 {declared} 处，全部落在 code/ 下")

    # ---- 5. 顶层节 ----
    for p in doc_files:
        text = p.read_text(encoding="utf-8")
        heads = re.findall(r"^## (.+)$", text, re.M)
        if len(heads) < 2:
            rep.fail(f"{p.name} 顶层节只有 {len(heads)} 个 —— 不足两节不成文章")
            continue
        ordinals = []
        bad = []
        for h in heads:
            head = h.split("、", 1)[0].strip()
            val = cn_ordinal(head)
            if val is None:
                bad.append(h)
            else:
                ordinals.append(val)
        if bad:
            rep.warn(f"{p.name} 有 {len(bad)} 个顶层节未用中文序号：{'；'.join(bad[:3])}")
        if ordinals and ordinals != list(range(ordinals[0], ordinals[0] + len(ordinals))):
            rep.warn(f"{p.name} 顶层节序号不连续：{ordinals}")

    # ---- 6. 代码块来源标注 ----
    unannotated = []
    total_blocks = 0
    for p in doc_files:
        lines = p.read_text(encoding="utf-8").splitlines()
        for start, lang, body in fenced_blocks(lines):
            if lang not in CODE_LANGS:
                continue
            total_blocks += 1
            head = " ".join(header_comment(body))
            if not any(m in head for m in PROVENANCE_MARKERS):
                first = next((l.strip() for l in body if l.strip()), "")
                unannotated.append(f"{p.name}:{start} [{lang}] {first[:70]}")
    if unannotated:
        for u in unannotated:
            rep.warn(f"代码块无来源标注：{u}")
    if total_blocks:
        rep.ok(f"代码块 {total_blocks} 个，其中 {len(unannotated)} 个待补来源标注")

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

#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""check_state.py —— 校验 STATE.md 是否满足 AGENTS.md §3 的"无悬空态"要求。

用法:
    python check_state.py <项目根目录>            # 全量校验
    python check_state.py <项目根目录> --resume   # 只打印恢复卡（仍做校验）

退出码:
    0 = 通过（可能有 [!] 警告）
    1 = 存在 [!!] 必须修的问题
    2 = 用法/文件错误

设计取舍：本脚本只做**可机械判定**的检查。像"意图是否单一""结构是否真的自足"
这类需要判断的项，脚本不假装能判——留给 AGENTS.md §6 的人工检查。
"""
import os
import re
import subprocess
import sys

# ---- 常量 ----------------------------------------------------------------

REQUIRED_KEYS = ["project", "updated", "phase", "step_id", "status", "head", "verify"]
VALID_STATUS = {"pending", "active", "done", "interrupted"}
REQUIRED_SECTIONS = ["已完成", "下一步", "阻塞", "决策", "待拍板"]
STATE_NAME = "STATE.md"

FAIL = "[!!]"
WARN = "[!] "
OK = "[ok]"


def out(tag, msg):
    print(f"{tag} {msg}")


# ---- 读取与解析 ----------------------------------------------------------

def read_text(path):
    with open(path, "r", encoding="utf-8") as f:
        return f.read()


def parse_front(text):
    """解析 --- 包起来的 front block，返回 dict 和正文起点行号。"""
    lines = text.splitlines()
    if not lines or lines[0].strip() != "---":
        return None, 0
    end = None
    for i in range(1, len(lines)):
        if lines[i].strip() == "---":
            end = i
            break
    if end is None:
        return None, 0
    data = {}
    for line in lines[1:end]:
        s = line.strip()
        if not s or s.startswith("#"):
            continue
        if ":" not in s:
            continue
        k, v = s.split(":", 1)
        data[k.strip()] = v.strip()
    return data, end + 1


def section_body(text, title):
    """取 '## <title>' 到下一个 '## ' 之间的正文；找不到返回 None。"""
    lines = text.splitlines()
    start = None
    for i, ln in enumerate(lines):
        if re.match(r"^##\s+", ln) and title in ln:
            start = i + 1
            break
    if start is None:
        return None
    buf = []
    for ln in lines[start:]:
        if re.match(r"^##\s+", ln):
            break
        buf.append(ln)
    return "\n".join(buf)


def strip_comments(text):
    return re.sub(r"<!--.*?-->", "", text, flags=re.S)


def bullet_items(body):
    """取出实质 bullet（忽略 none / 空 / 注释）。"""
    body = strip_comments(body)
    items = []
    for ln in body.splitlines():
        s = ln.strip()
        if not s.startswith(("- ", "* ")):
            continue
        val = s[2:].strip()
        if not val or val.lower() in ("none", "无", "n/a", "-"):
            continue
        items.append(val)
    return items


# ---- git ----------------------------------------------------------------

def git(root, *args):
    try:
        r = subprocess.run(["git", "-C", root] + list(args),
                           capture_output=True, text=True, timeout=15)
    except (OSError, subprocess.SubprocessError):
        return None
    if r.returncode != 0:
        return None
    return r.stdout


# ---- 主校验 --------------------------------------------------------------

def main(argv):
    if len(argv) < 2:
        out(FAIL, "用法: python check_state.py <项目根目录> [--resume]")
        return 2

    root = os.path.abspath(argv[1])
    resume_only = "--resume" in argv[2:]

    state_path = os.path.join(root, STATE_NAME)
    if not os.path.isfile(state_path):
        out(FAIL, f"{STATE_NAME} 不存在于 {root}")
        out("  ", "→ 按 AGENTS.md §1 初始化 STATE.md，不要直接开工（从未初始化 ≠ 可以开工）")
        return 1

    text = read_text(state_path)
    fails, warns = [], []

    # 1) front block
    front, _ = parse_front(text)
    if front is None:
        fails.append("缺少 --- 包裹的 front block")
        front = {}
    else:
        missing = [k for k in REQUIRED_KEYS if not front.get(k)]
        if missing:
            fails.append(f"front block 缺字段: {', '.join(missing)}")

    status = front.get("status", "")
    if status and status not in VALID_STATUS:
        fails.append(f"status='{status}' 非法，应为 {'/'.join(sorted(VALID_STATUS))}")

    # 2) 章节
    for sec in REQUIRED_SECTIONS:
        if section_body(text, sec) is None:
            fails.append(f"缺少章节: ## {sec}")

    # 3) 下一步必须唯一
    nxt = section_body(text, "下一步")
    if nxt is not None:
        actions = re.findall(r"^\s*[-*]\s*动作\s*[:：]", strip_comments(nxt), re.M)
        if len(actions) == 0:
            fails.append("『下一步』未给出可执行动作（缺 `- 动作：` 行）")
        elif len(actions) > 1:
            fails.append(f"『下一步』有 {len(actions)} 条，协议要求恰好 1 条")

    # 4) 待拍板 ≤ 1
    pending = section_body(text, "待拍板")
    if pending is not None:
        n = len(bullet_items(pending))
        if n > 1:
            fails.append(f"『待拍板』有 {n} 条，协议要求 ≤1（多条说明上一步该拆）")

    # 5) verify 非空且不是占位符
    verify = front.get("verify", "")
    if verify and re.search(r"[<>]", verify):
        fails.append(f"verify 仍是占位符: {verify}")

    # 6) git 对齐
    #
    # 设计说明（重要，别"简化"掉）：
    #   STATE.md 自己也要提交，提交后 HEAD 必然前移，于是"head == git log -1"会变成死锁。
    #   解决办法是把状态提交单独标记出来：约定同步 STATE.md 的那次提交用 `chore(state):` 前缀。
    #   校验放宽为：head == tip，或 (tip 是 chore(state) 提交 且 head == tip 的父提交)。
    #   这样既不牺牲"能把状态锚到具体提交"的能力，也不逼人 amend 出无法预知的 hash。
    STATE_PREFIX = "chore(state)"

    log = git(root, "log", "-3", "--format=%h|%s")
    tip_hash = tip_subject = None
    prev_hash = None
    if log:
        rows = [l for l in log.strip().splitlines() if l.strip()]
        if rows:
            parts = rows[0].split("|", 1)
            tip_hash = parts[0].strip()
            tip_subject = (parts[1] if len(parts) > 1 else "").strip()
        if len(rows) > 1:
            prev_hash = rows[1].split("|", 1)[0].strip()

    dirty = git(root, "status", "--short")

    if tip_hash is None:
        warns.append("不是 git 仓库或 git 不可用 —— 无断点回退能力，建议尽快 git init")
    else:
        head = front.get("head", "")
        head_hash = head.split()[0] if head else ""
        if not head_hash:
            fails.append("head 为空 —— 恢复时无法把状态与磁盘对齐")
        elif head_hash == tip_hash:
            pass
        elif tip_subject and tip_subject.startswith(STATE_PREFIX) and head_hash == prev_hash:
            pass  # 正常的"工作提交 + 状态提交"两段式
        else:
            fails.append(
                f"head({head_hash}) 与 git log -1({tip_hash} {tip_subject}) 不一致，且 tip 不是 "
                f"{STATE_PREFIX}: 提交 —— 有未记录的工作，先查明再继续")
        if dirty and dirty.strip():
            n = len([l for l in dirty.strip().splitlines() if l.strip()])
            fails.append(f"工作树有 {n} 个未提交改动 —— 违反停机铁律『无悬空态』")

    # ---- 输出 ----
    if resume_only:
        print("=" * 56)
        print(f"【恢复】{front.get('project', '?')} / {front.get('phase', '?')} / "
              f"step {front.get('step_id', '?')} status={status or '?'}")
        print(f"更新于: {front.get('updated', '?')}")
        if tip_hash is not None:
            clean = "工作树干净" if not (dirty or "").strip() else "工作树不干净"
            print(f"git  : tip={tip_hash} {clean}  |  {tip_subject or ''}")
            if front.get("head"):
                print(f"       状态锚点 head={front['head']}")
        else:
            print("git  : 不可用（无断点回退能力）")
        print(f"验证 : {verify or '（缺）'}")
        print("-" * 56)
        if nxt is not None:
            acts = [l.strip() for l in strip_comments(nxt).splitlines() if l.strip()]
            for l in acts:
                print(f"  {l}")
        print("-" * 56)
        print(f"待拍板: {len(bullet_items(pending)) if pending is not None else '（缺章节）'}")
        print("=" * 56)
        print("⚠ 校验见下；有 [!!] 时不许开工。恢复阶段不产出内容。")

    for f in fails:
        out(FAIL, f)
    for w in warns:
        out(WARN, w)

    if not fails:
        out(OK, f"{STATE_NAME} 通过机械校验"
                f"（step {front.get('step_id', '?')} / status={status or '?'}）")
        out("  ", "仍需人工过 AGENTS.md §6：意图单一、结构自足、机械改动是否分离")

    return 1 if fails else 0


if __name__ == "__main__":
    try:
        sys.stdout.reconfigure(encoding="utf-8")
    except AttributeError:
        pass
    sys.exit(main(sys.argv))

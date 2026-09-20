#!/usr/bin/env bash
# sync.sh —— 把 small-step-loop 技能真身同步到本仓库副本
#
# 方向：真身（Windows 侧 WorkBuddy 目录）→ 本仓库 skills/small-step-loop
# 真身是**唯一编辑入口**；仓库里的副本只读，不直接改。
# 用法：bash skills/sync.sh
set -euo pipefail

SRC="/mnt/c/Users/husci/.workbuddy/skills/small-step-loop"
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DST="$HERE/small-step-loop"

[ -f "$SRC/SKILL.md" ] || { echo "✗ 找不到真身: $SRC" >&2; exit 2; }
[ -d "$DST" ]          || { echo "✗ 找不到副本目录: $DST" >&2; exit 2; }

rm -rf "$DST"
mkdir -p "$DST"
cp -r "$SRC/." "$DST/"
find "$DST" -type d -name __pycache__ -prune -exec rm -rf {} + 2>/dev/null || true

echo "✓ 已同步: $SRC → $DST"
diff -r "$SRC" "$DST" >/dev/null && echo "✓ 校验通过：两侧内容一致"

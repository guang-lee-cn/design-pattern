#!/usr/bin/env bash
# install.sh —— 把本技能接到其他 AI 平台（真身仍在 ~/.workbuddy/skills/small-step-loop）
#
# 幂等：可重复执行；只创建软链，不复制、不覆盖真身。
# 用法：
#   bash install.sh                      # 自动探测所有已知平台目录
#   bash install.sh /path/to/skillsdir   # 指定单一目标目录
#
# ⚠️ 本脚本只在 WSL / Linux 侧执行。真身在 Windows 的 WorkBuddy 目录下，
#    从 WSL 访问它的路径是 /mnt/c/Users/husci/.workbuddy/skills/small-step-loop。

set -euo pipefail

SRC_CANDIDATES=(
  "/mnt/c/Users/husci/.workbuddy/skills/small-step-loop"
  "$HOME/.workbuddy/skills/small-step-loop"
)

SRC=""
for c in "${SRC_CANDIDATES[@]}"; do
  if [ -f "$c/SKILL.md" ]; then SRC="$c"; break; fi
done

if [ -z "$SRC" ]; then
  echo "✗ 找不到真身 SKILL.md，检查过：" >&2
  printf '  %s\n' "${SRC_CANDIDATES[@]}" >&2
  exit 2
fi
echo "真身: $SRC"

if [ "$#" -ge 1 ]; then
  TARGETS=("$1")
else
  TARGETS=(
    "$HOME/.claude/skills"
    "$HOME/.codex/skills"
    "$HOME/.gemini/skills"
  )
fi

for t in "${TARGETS[@]}"; do
  mkdir -p "$t"
  link="$t/small-step-loop"
  if [ -L "$link" ]; then
    cur=$(readlink "$link")
    if [ "$cur" = "$SRC" ]; then
      echo "  = 已接线: $link"
      continue
    fi
    rm -f "$link"
  elif [ -e "$link" ]; then
    echo "  ! 跳过（已存在实体目录，非软链）: $link" >&2
    continue
  fi
  ln -sfn "$SRC" "$link"
  echo "  + 已接线: $link -> $SRC"
done

echo
echo "自检（应无输出 = 无悬空软链）:"
find "$HOME/.claude/skills" -type l ! -exec test -e {} \; -print 2>/dev/null || true

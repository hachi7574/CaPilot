#!/usr/bin/env bash
# ============================================================
# setup-skill-links.sh
# 跨平台 Skill 链接脚本 — 让所有 agent 共享 Skill/ 单一数据源
#
# 用法:
#   bash Skill/Skill-Sync-Scripts/setup-skill-links.sh          # 创建/修复链接
#   bash Skill/Skill-Sync-Scripts/setup-skill-links.sh --clean   # 删除所有链接（不删源文件）
#
# 原理:
#   Linux  → symlink
#   Windows→ 目录联接 (mklink /J, 不需管理员权限)
#
# 支持的 agent 目录（自动检测存在的）:
#   .claude/skills/    (Claude Code)
#   .codebuddy/skills/ (CodeBuddy)
#   .agents/skills/    (通用)
#   .cursor/skills/    (Cursor)
#   .windsurf/skills/  (Windsurf)
# ============================================================

set -euo pipefail

# --- 定位项目根 ---
# 脚本位于 Skill/Skill-Sync-Scripts/，需上跳两级到项目根
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
SKILL_SOURCE="$PROJECT_ROOT/Skill"

# --- 配置: 支持的 agent skill 目录 ---
AGENT_DIRS=(
  ".claude/skills"
  ".codebuddy/skills"
  ".agents/skills"
  ".cursor/skills"
  ".windsurf/skills"
)

# --- 检测 OS ---
detect_os() {
  case "$(uname -s)" in
    Linux*)  echo "linux" ;;
    Darwin*) echo "macos" ;;
    MINGW*|MSYS*|CYGWIN*) echo "windows" ;;
    *) echo "unknown" ;;
  esac
}

OS="$(detect_os)"

# --- 创建链接的核心函数 ---
create_link() {
  local target="$1"   # source dir (absolute path)
  local link_path="$2" # link path (absolute path)

  if [ "$OS" = "windows" ]; then
    # Windows (Git Bash): test commands can't detect Junctions
    # Use Python for everything. Must convert paths to Windows format.
    # Note: try 'python' before 'python3' — Windows Store has a python3 stub
    local python_bin win_link win_target
    python_bin="$(command -v python 2>/dev/null || command -v python3 2>/dev/null)"
    if [ -z "$python_bin" ]; then
      echo "  [ERROR] python not found"
      return 1
    fi
    win_link="$(cygpath -w "$link_path" 2>/dev/null || echo "$link_path")"
    win_target="$(cygpath -w "$target" 2>/dev/null || echo "$target")"

    # All-in-one: remove existing + create Junction
    "$python_bin" -c "
import os, sys, shutil, subprocess

link = sys.argv[1]
target = sys.argv[2]

# Remove existing link/Junction/dir
if os.path.exists(link) or os.path.islink(link):
    if os.path.islink(link):
        os.unlink(link)
    elif os.path.isdir(link):
        try:
            os.rmdir(link)
        except OSError:
            import time
            backup = link + '.bak.' + str(int(time.time()))
            shutil.move(link, backup)
            print(f'  [BACKUP] old copy -> {backup}')

# Create Junction
result = subprocess.run(
    ['cmd', '/c', 'mklink', '/J', link, target],
    capture_output=True
)
if result.returncode == 0 and os.path.exists(link):
    print('  [OK]')
else:
    err = result.stderr.decode('gbk', errors='replace').strip()
    print(f'  [FAIL] {err}')
    sys.exit(1)
" "$win_link" "$win_target"
    return $?
  else
    # Linux/macOS: standard symlink detection
    if [ -L "$link_path" ]; then
      rm "$link_path"
    elif [ -d "$link_path" ]; then
      local backup="${link_path}.bak.$(date +%s)"
      echo "  [BACKUP] old copy -> $backup"
      mv "$link_path" "$backup"
    elif [ -e "$link_path" ]; then
      echo "  [SKIP] $link_path exists but not dir/link"
      return 1
    fi
    ln -s "$target" "$link_path"
    echo "  [SYMLINK] $link_path -> $target"
    return 0
  fi
}

# --- 清理模式 ---
if [ "${1:-}" = "--clean" ]; then
  echo "=== Clean all agent skill links ==="
  if [ "$OS" = "windows" ]; then
    # Windows: use Python to detect and remove Junctions
    python_bin="$(command -v python 2>/dev/null || command -v python3 2>/dev/null)"
    if [ -z "$python_bin" ]; then
      echo "[ERROR] python not found"
      exit 1
    fi
    for agent_dir in "${AGENT_DIRS[@]}"; do
      full_dir="$PROJECT_ROOT/$agent_dir"
      [ -d "$full_dir" ] || continue
      win_full_dir="$(cygpath -w "$full_dir" 2>/dev/null || echo "$full_dir")"
      "$python_bin" -c "
import os, sys
d = sys.argv[1]
for name in os.listdir(d):
    p = os.path.join(d, name)
    if os.path.islink(p):
        os.unlink(p)
        print(f'  [REMOVED] {name}')
    elif os.path.isdir(p):
        try:
            os.rmdir(p)
            print(f'  [REMOVED] {name}')
        except OSError:
            pass  # real dir, skip
" "$win_full_dir"
    done
  else
    # Linux/macOS: standard symlink removal
    for agent_dir in "${AGENT_DIRS[@]}"; do
      full_dir="$PROJECT_ROOT/$agent_dir"
      [ -d "$full_dir" ] || continue
      for skill_link in "$full_dir"/*; do
        [ -e "$skill_link" ] || continue
        if [ -L "$skill_link" ]; then
          rm "$skill_link"
          echo "  [REMOVED] $(basename "$skill_link")"
        fi
      done
    done
  fi
  echo "Done. Skill/ source files not affected."
  exit 0
fi

# --- 主逻辑 ---
echo "=== setup-skill-links ==="
echo "OS: $OS"
echo "项目根: $PROJECT_ROOT"
echo "Skill 源: $SKILL_SOURCE"
echo ""

# 检查 Skill/ 存在
if [ ! -d "$SKILL_SOURCE" ]; then
  echo "[ERROR] Skill/ 目录不存在: $SKILL_SOURCE"
  exit 1
fi

# 列出所有 skill（排除 Skill-Sync-Scripts 本身）
SKILL_DIRS=()
for d in "$SKILL_SOURCE"/*/; do
  [ -d "$d" ] || continue
  name="$(basename "$d")"
  # 排除同步脚本目录本身
  [ "$name" = "Skill-Sync-Scripts" ] && continue
  SKILL_DIRS+=("$name")
done

if [ ${#SKILL_DIRS[@]} -eq 0 ]; then
  echo "[ERROR] Skill/ 下没有 skill 目录"
  exit 1
fi

echo "发现 ${#SKILL_DIRS[@]} 个 skill: ${SKILL_DIRS[*]}"
echo ""

# 为每个 agent 目录创建链接
created=0
failed=0
for agent_dir in "${AGENT_DIRS[@]}"; do
  full_dir="$PROJECT_ROOT/$agent_dir"

  # 只处理已存在的 agent 目录（不自动创建）
  if [ ! -d "$full_dir" ]; then
    continue
  fi

  echo ">> $agent_dir"

  for skill_name in "${SKILL_DIRS[@]}"; do
    target="$SKILL_SOURCE/$skill_name"
    link_path="$full_dir/$skill_name"

    if create_link "$target" "$link_path"; then
      created=$((created + 1))
    else
      failed=$((failed + 1))
    fi
  done
  echo ""
done

echo "=== 完成 ==="
echo "成功: $created  失败: $failed"
if [ $failed -gt 0 ]; then
  echo ""
  echo "有链接创建失败，请检查上方日志。"
  echo "Windows 下如遇权限问题，可尝试以管理员身份运行或开启 Developer Mode。"
  exit 1
fi

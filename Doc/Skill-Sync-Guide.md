# Skill 同步机制使用说明

## 问题背景

项目中多个 AI agent（Claude Code、CodeBuddy、Cursor 等）各自有不同的 skill 目录约定：

| Agent | 默认 skill 目录 |
|-------|----------------|
| Claude Code | `.claude/skills/` |
| CodeBuddy | `.codebuddy/skills/` |
| Cursor | `.cursor/skills/` |
| Windsurf | `.windsurf/skills/` |
| 通用 | `.agents/skills/` |

如果每个目录各存一份 skill 副本，随着时间推移必然分叉——某个 agent 修了 bug，另一个 agent 还在用旧版。

## 解决方案

**`Skill/` 是唯一数据源**，所有 agent 的 skill 目录通过**目录联接**透明指向它。

```
项目根/
├─ Skill/                        ← 唯一数据源（git 追踪）
│   ├─ Esp-Build-Skill/
│   ├─ Esp-Component-Skill/
│   ├─ Esp-Datasheet-Skill/      ← 芯片手册管理（PDF→Markdown + 检索）
│   └─ Skill-Sync-Scripts/       ← 链接脚本（git 追踪）
│       ├─ setup-skill-links.sh  ← Linux/macOS/Git Bash
│       └─ setup-skill-links.ps1 ← Windows PowerShell
├─ .claude/skills/               ← 自动生成（gitignore）
│   ├─ Esp-Build-Skill → ../../Skill/Esp-Build-Skill
│   ├─ Esp-Component-Skill → ../../Skill/Esp-Component-Skill
│   └─ Esp-Datasheet-Skill → ../../Skill/Esp-Datasheet-Skill
├─ .codebuddy/skills/            ← 同理（如果存在）
└─ .agents/skills/               ← 同理（如果存在）
```

### 链接类型

| OS | 链接类型 | 实现方式 | 需要管理员？ |
|----|---------|---------|-------------|
| Linux | symlink | `ln -s` | 否 |
| macOS | symlink | `ln -s` | 否 |
| Windows (Git Bash) | Junction | Python subprocess → `cmd /c mklink /J` | 否 |
| Windows (PowerShell) | Junction | Python subprocess → `cmd /c mklink /J` | 否 |

Junction 比 symlink 的优势：不需要管理员权限或 Developer Mode，对应用层完全透明。

> **Windows 注意事项：**
> - Git Bash 的 `test` 命令无法检测 Junction，因此脚本统一用 Python 处理 junction 的检测、删除和创建
> - 脚本优先查找 `python` 而非 `python3`（Windows Store 的 `python3` 是空壳 stub）
> - `cygpath -w` 用于将 Git Bash 路径转换为 Windows 路径

## 使用方法

### 新环境初始化（clone 项目后）

**Linux / macOS / Git Bash:**

```bash
bash Skill/Skill-Sync-Scripts/setup-skill-links.sh
```

**Windows PowerShell:**

```powershell
.\Skill\Skill-Sync-Scripts\setup-skill-links.ps1
```

脚本会：
1. 扫描 `Skill/` 下所有 skill 目录（排除 `Skill-Sync-Scripts` 本身）
2. 检测已存在的 agent 目录（`.claude/skills/`、`.codebuddy/skills/` 等）
3. 为每个 agent 目录创建指向 `Skill/` 的链接（已有链接会先删除再重建）
4. 如果发现旧的真实目录副本，自动备份为 `*.bak.<timestamp>` 再创建链接

### 新增 agent 目录后

如果新建了 `.cursor/skills/` 之类的目录，重跑脚本即可：

```bash
bash Skill/Skill-Sync-Scripts/setup-skill-links.sh
```

已有的链接会删除后重建，只为已存在的 agent 目录创建链接。

### 新增 skill 后

在 `Skill/` 下创建新 skill 目录后，重跑脚本即可自动为所有 agent 目录创建链接。

### 清理链接

**Linux / macOS / Git Bash:**

```bash
bash Skill/Skill-Sync-Scripts/setup-skill-links.sh --clean
```

**Windows PowerShell:**

```powershell
.\Skill\Skill-Sync-Scripts\setup-skill-links.ps1 -Clean
```

只删除链接/Junction，不删 `Skill/` 源文件。

## 修改 Skill 的规则

- **只改 `Skill/` 下的文件**，因为这是唯一数据源
- 修改后所有 agent 立即生效（同一物理目录，无需同步）
- 不要直接改 `.claude/skills/` 下的文件——它们是链接，改了等于改源文件，但容易产生混淆

## SKILL.md 路径规范

所有 SKILL.md 中的路径使用 `{{PROJECT_ROOT}}` 占位符，不硬编码绝对路径：

```markdown
# 正确
& "{{PROJECT_ROOT}}/Skill/Esp-Build-Skill/scripts/build.ps1"

# 错误
& "B:\CaPilot\Skill\Esp-Build-Skill\scripts\build.ps1"
```

Agent 加载 skill 后，将 `{{PROJECT_ROOT}}` 替换为实际项目根路径即可。

脚本需要 Python 环境（用于 Windows Junction 创建）。Windows 上优先使用 `python`（非 `python3`），Linux/macOS 使用 `python3` 或 `python` 均可。

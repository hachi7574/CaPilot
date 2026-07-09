---
name: esp-datasheet-skill
description: "芯片手册管理：PDF→结构化Markdown转换（Marker引擎，保留标题/表格/LaTeX/图片/Caption/中文）+ Markdown检索 + raw-pdf fallback。使用场景：查寄存器、确认ODR/量程编码、搜索关键词、转换手册。下游skill（ESP-COMPONENT-SKILL）优先检索Markdown而非裸读PDF。"
agent_created: true
---

# ESP-DATASHEET-SKILL — 芯片手册管理

将芯片手册 PDF 转为结构化 Markdown，提供高效检索。下游 skill（如 ESP-COMPONENT-SKILL）**优先检索 Markdown**，不再直接读 PDF。

## 触发

用户说"查手册"、"查寄存器"、"转换手册"、"搜索 CTRL2"、"QMI8658 的 ODR 怎么配"、"芯片手册转 Markdown"。

## 核心理念

| 旧方式（ESP-PDF-SKILL） | 新方式（本 Skill） |
|------------------------|-------------------|
| 每次查手册都裸读 PDF | PDF 一次性转为 Markdown，后续只检索 Markdown |
| 上下文消耗大（PDF 文本冗余） | 上下文消耗小（Markdown 精简） |
| 表格/公式/图片丢失 | 保留表格(GFM)、公式(LaTeX)、图片(alt) |
| 检索精度低（纯文本匹配） | 检索精度高（结构化标题/段落） |
| 无法增量维护 | Markdown 可手动修正/补充 |

## 手册位置

```
{{PROJECT_ROOT}}/Hardware/Lceda-Esp32-S3-Practical-Board/Docs/02-Chip-Manuals/
├── QMI8658A.PDF       (六轴 IMU)
├── ES7210.pdf         (麦克风 ADC)
├── ES8311.pdf         (音频 DAC)
├── CH340K.pdf         (USB 转串口)
├── PCA9557.pdf        (I2C GPIO 扩展)
├── NS4150B.pdf        (音频功放)
├── ch334f.pdf         (USB Hub)
├── ZTS6216.pdf        (触摸芯片)
└── esp32-s3-wroom-1.pdf (主控模组)
```

## 转换后 Markdown 位置

```
{{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/datasheets/
├── _index.md               ← 总索引（自动生成）
├── QMI8658A/
│   ├── README.md           ← 元数据
│   ├── full.md             ← 完整 Markdown（含表格/公式/图片引用）
│   └── images/             ← 提取的图片
├── ES7210/
│   └── ...
└── ...
```

## 脚本位置

```
{{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/scripts/convert_pdf.py
```

## 依赖

- Python: marker-pdf, pymupdf
- 运行命令: `python`（Windows）或 `python3`（Linux）
- 环境已配置在 managed Python venv 中

## AI 使用方式

### 1. 转换 PDF → Markdown（Marker 引擎）

```bash
# 转换单个手册
python "{{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/scripts/convert_pdf.py" convert \
  "{{PROJECT_ROOT}}/Hardware/.../QMI8658A.PDF"

# 转换所有手册（一键，自动跳过已转换的）
python "{{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/scripts/convert_pdf.py" convert --all

# 强制重新转换所有手册（覆盖已有 Markdown）
python "{{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/scripts/convert_pdf.py" convert --all --force
```

转换会保留：
- 标题层级（`#`/`##`/`###`）
- 表格（GFM `| col | col |` 格式）
- LaTeX 公式（`$$...$$` 或 `$...$`）
- 图片（保存到 `images/`，Markdown 中以 `![](images/xxx.png)` 引用）
- Caption（图注/表注）
- 中文文本

### 2. 检索已转换的 Markdown（推荐，优先使用）

```bash
# 在某芯片的 Markdown 中搜索关键词
python "{{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/scripts/convert_pdf.py" search-md \
  QMI8658A "CTRL2"
```

搜索结果会显示匹配行号 + 上下文（前后各 2 行）。

### 3. raw-pdf 模式（fallback，直接读 PDF）

当某芯片尚未转换、或 Markdown 中缺少信息时，可直接读 PDF：

```bash
# PDF 基本信息
python ".../convert_pdf.py" info "{{PROJECT_ROOT}}/Hardware/.../QMI8658A.PDF"

# 搜索关键词（原始 PDF）
python ".../convert_pdf.py" search "{{PROJECT_ROOT}}/Hardware/.../QMI8658A.PDF" "CTRL2"

# 提取指定页文本
python ".../convert_pdf.py" page "{{PROJECT_ROOT}}/Hardware/.../QMI8658A.PDF" 31

# 提取表格
python ".../convert_pdf.py" tables "{{PROJECT_ROOT}}/Hardware/.../QMI8658A.PDF" 31
```

### 4. 重建索引

```bash
python "{{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/scripts/convert_pdf.py" build-index
```

## 典型工作流

### 场景 A：开发新驱动组件（与 ESP-COMPONENT-SKILL 协作）

```
1. 确认目标芯片 → 检查 datasheets/{chip}/ 是否已转换
   - 已转换 → 直接 search-md 检索寄存器信息
   - 未转换 → 先 convert 转换，再 search-md

2. 查寄存器：search-md {chip} "CTRL2" → 获取位域定义
3. 确认编码：search-md {chip} "ODR" → 获取 ODR 编码表
4. 如 Markdown 缺信息 → fallback 到 raw-pdf 模式直接读 PDF 验证
5. 在组件代码中注释手册页码 + 寄存器名
```

### 场景 B：排查寄存器配置问题

```
1. search-md {chip} "CTRL2"     → 看位域定义
2. search-md {chip} "Self-Test" → 确认 bit7 含义
3. 对比代码中的寄存器值
4. 如有歧义 → raw-pdf page 31 看原始页面
```

### 场景 C：PDF 更新后重新转换

```bash
# 单个重转
python ".../convert_pdf.py" convert "{{PROJECT_ROOT}}/Hardware/.../QMI8658A.PDF"
# 索引自动更新
```

## 与其他 Skill 的协作

| Skill | 关系 |
|-------|------|
| **ESP-COMPONENT-SKILL** | 下游消费者。开发驱动组件时优先检索本 skill 的 Markdown，不再直接读 PDF |
| ESP-BUILD-SKILL | 无直接关系 |
| Skill-Sync-Scripts | 本 skill 通过同步机制链接到各 agent 目录 |

**ESP-COMPONENT-SKILL 中的引用已更新为：**
1. 优先 `search-md` 检索 Markdown
2. Fallback 到 `raw-pdf` 模式
3. 不再引用独立的 ESP-PDF-SKILL（已合并）

## 注意事项

- Marker 转换需要 ML 模型，首次运行会下载模型（~1GB），后续转换不再下载
- CPU 模式下转换一本 3MB 手册约 2-5 分钟，9 本手册约 15-30 分钟
- 部分扫描版 PDF 无文本层，Marker 会自动 OCR 但质量可能下降
- Markdown 可手动修正（如表格对齐、公式修正），重转会覆盖
- 图片文件名由 Marker 自动生成，可能需要人工重命名为有意义的名称

## 旧 ESP-PDF-SKILL 迁移说明

本 skill 已合并原 `ESP-PDF-SKILL` 的全部功能：
- `pdf_reader.py` 的 `info/toc/search/page/pages/tables` → 本 skill 的 raw-pdf 模式
- 独立的 `ESP-PDF-SKILL/` 目录已删除
- 所有引用已更新指向本 skill

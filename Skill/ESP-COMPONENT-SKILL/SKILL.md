---
name: esp-component-create
description: "ESP-IDF 可复用组件开发工作流。设计原则：高内聚、低耦合、可复用、可单独测试/编译/Demo验证、不依赖业务逻辑。8步开发流程：需求分析→架构设计→API设计→内部架构→实现→独立Demo→验证→测试报告。使用场景：用户说'做组件'/'封装 XX 功能'/'新建 ESP-IDF 工程'。"
agent_created: true
---

# ESP-IDF 可复用组件开发工作流

从零设计并实现 ESP-IDF 可复用组件，遵循统一设计原则和开发流程。

## 触发

用户说"做组件"、"封装 XX 功能"、"新建 ESP-IDF 工程"、"USB HID"、"BLE HID"、"加个模块"、"驱动 XX 芯片"。

## 一、设计哲学

每个组件必须满足以下 7 条原则：

| 原则 | 含义 | 反例 |
|------|------|------|
| **高内聚** | 组件内所有代码只为一个明确职责服务 | 陀螺仪组件里混入 LCD 初始化 |
| **低耦合** | 组件之间只通过公开 API 交互，不依赖彼此内部实现 | `extern` 跨组件访问 static 变量 |
| **可复用** | 换一个项目、换一个应用场景，不改组件代码就能用 | `if (app_is_keyboard_mode())` 写在组件里 |
| **可单独测试** | 不依赖完整应用也能跑单元测试 | 组件 init 需要 main.c 先初始化 5 个外设 |
| **可单独编译** | 组件 + 最小 main.c 就能通过编译 | 组件 REQUIRES 里漏写依赖、靠上层工程补 |
| **可独立 Demo 验证** | 有独立 Demo 工程，只依赖该组件 + 硬件最小系统 | 没有 Demo，只能嵌在主应用里"顺便验证" |
| **不依赖业务逻辑** | 组件不知道"产品要做什么"，只知道"硬件能做什么" | 组件里有手语识别、键盘布局、宠物动画逻辑 |

## 二、禁区清单

以下内容**严禁**出现在组件代码中：

| 禁区 | 说明 | 正确做法 |
|------|------|----------|
| Demo 代码 | `demo_main.c` 里的示例逻辑 | Demo 是独立工程，不放在 `components/` 里 |
| 业务逻辑 | 手语识别、键盘映射、动画状态机 | 放在 `main/` 的应用层 |
| UI 逻辑 | LVGL 控件、屏幕布局、页面切换 | 放在 `main/app_ui.c` |
| AI 逻辑 | 模型推理、语音识别、姿态解算 | 放在独立 AI 组件或应用层 |
| 硬编码接口 | `void set_servo_angle(int ch, float deg)` — 只满足当前需求 | 先分析硬件能力，再设计通用 API |
| 跨组件内部访问 | Component A 直接读 Component B 的 `static` 变量或未公开结构体 | 通过 B 的公开 API 交互 |

**判断标准**：如果删掉 `main/` 下所有业务代码，组件是否仍然完整、可编译、可通过 Demo 验证？答"是"才算合格。

## 三、统一 Coding Style

所有组件代码必须遵循项目统一的 C 代码风格，详见模板文件：

```
{{PROJECT_ROOT}}/Skill/Esp-Component-Skill/templates/component_coding_style.md
```

核心规则速查：

- **命名**: 函数/变量 `snake_case`，宏 `UPPER_SNAKE`，类型 `typedef ... _t`
- **头文件**: `#pragma once` + `extern "C"` 包裹
- **日志**: 统一用 `ESP_LOGI/W/E/D`，TAG 格式 `"[comp_name]"`
- **错误处理**: 所有公开 API 返回 `esp_err_t`，内部致命错误用 `ESP_ERROR_CHECK`
- **初始化**: `init(config_t *cfg)` → `deinit()` 配对，支持反复 init/deinit
- **常量**: 用 `#define` 或 `enum`，不做 magic number
- **文件编码**: UTF-8 without BOM
- **缩进**: 4 空格，不用 Tab

## 四、芯片手册引用（ESP-DATASHEET-SKILL）

当组件驱动某个**外设芯片**（如 QMI8658、ES8311、PCA9557）时，**必须**查阅对应的芯片寄存器手册。这是硬性要求，不是建议。

### 如何查手册

手册位置：
```
{{PROJECT_ROOT}}/Hardware/Lceda-Esp32-S3-Practical-Board/Docs/02-Chip-Manuals/
```

查阅工具：**ESP-DATASHEET-SKILL**（位于 `{{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/`）

该 skill 将 PDF 转为结构化 Markdown（保留标题/表格/LaTeX/图片/Caption/中文），**优先检索 Markdown**，大幅降低上下文消耗。

#### 步骤 1：确认目标芯片是否已转换

```bash
ls {{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/datasheets/
# 如果看到 {芯片名}/ 目录 → 已转换，直接检索
# 如果没有 → 先转换（见步骤 2）
```

#### 步骤 2：转换（仅在未转换时执行）

```bash
# 转换单个手册
python {{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/scripts/convert_pdf.py convert \
  {{PROJECT_ROOT}}/Hardware/.../QMI8658A.PDF

# 或一键转换所有手册
python {{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/scripts/convert_pdf.py convert --all
```

#### 步骤 3：检索 Markdown（推荐，优先使用）

```bash
# 搜索寄存器（如 QMI8658 的 CTRL2）— 在已转换的 Markdown 中搜索
python {{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/scripts/convert_pdf.py search-md \
  QMI8658A "CTRL2"
```

检索结果包含匹配行号 + 上下文，Markdown 中的表格/公式结构完整保留。

#### 步骤 4（可选 Fallback）：raw-pdf 模式

当 Markdown 缺少信息、或需查看原始 PDF 页面时：

```bash
# 直接搜索原始 PDF
python {{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/scripts/convert_pdf.py \
  search {{PROJECT_ROOT}}/Hardware/.../QMI8658A.PDF "CTRL2"

# 提取原始页面文本
python {{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/scripts/convert_pdf.py \
  page {{PROJECT_ROOT}}/Hardware/.../QMI8658A.PDF 31

# 提取表格
python {{PROJECT_ROOT}}/Skill/Esp-Datasheet-Skill/scripts/convert_pdf.py \
  tables {{PROJECT_ROOT}}/Hardware/.../QMI8658A.PDF 31
```

### 何时必须查手册

- 初始化配置（确认寄存器默认值、推荐配置、上电时序）
- 数据读取（确认 data-ready 位行为、数据格式、字节序）
- 模式切换（确认触发条件、切换延迟、状态机）
- 发现代码中的寄存器值与手册不一致

**检查点**：在"架构设计"阶段就要列出需要确认的寄存器清单。

## 五、8 步开发流程

以下流程为固定顺序，不可跳过任何一步。

```
需求分析  ──→  架构设计  ──→  API 设计  ──→  内部架构
   │               │              │              │
   ▼               ▼              ▼              ▼
[需求文档]    [架构图+寄存器清单]  [公开头文件]   [内部模块划分]

实现Component  ──→  独立 Demo  ──→  Demo 验证  ──→  测试报告
   │                  │               │               │
   ▼                  ▼               ▼               ▼
 [.c 源码]        [demo 工程]     [串口日志]     [测试报告.md]
```

### 第 1 步：需求分析

**产出物**：组件需求简述（口头或注释，不需要单独文档）

必须明确回答：
- 这个组件要驱动什么硬件 / 实现什么功能？
- 硬件的能力边界是什么？（查手册！）
- 哪些是硬件能力、哪些是业务需求？（划清边界）
- 输入什么？输出什么？
- 有多少个配置项？哪些运行时可改、哪些仅 init 时配？

### 第 2 步：架构设计

**产出物**：架构说明 + 寄存器确认清单

必须明确：
- 组件在系统中的位置（依赖谁、被谁依赖）
- 需要哪些 ESP-IDF 组件（I2C/SPI/UART/Timer/GPIO...）
- 需要确认的芯片寄存器清单（从手册中提取）
- 中断 vs 轮询的选择及理由
- 内存/Flash 预算（大致即可）

### 第 3 步：API 设计

**产出物**：公开头文件（`include/{name}.h`）

API 设计原则：
- 先设计接口，再写实现
- 每个函数只做一件事
- 配置参数用 `config_t` 结构体，不用一长串参数
- 提供 `init`/`deinit` 生命周期管理
- 异步操作提供回调注册，不阻塞调用者
- 避免 `void *user_data` 滥用——类型明确优先

必须提供的 API：

| API | 签名模式 | 说明 |
|-----|---------|------|
| 初始化 | `esp_err_t {name}_init(const {name}_config_t *cfg)` | 配置驱动 |
| 反初始化 | `esp_err_t {name}_deinit(void)` | 释放资源 |
| 就绪检查 | `bool {name}_is_ready(void)` | 硬件状态查询 |
| 核心功能 | 视组件而定 | 读数据、发命令、开关等 |

### 第 4 步：内部架构

**产出物**：内部模块划分（注释或设计说明）

明确：
- 哪些是 `static` 内部函数、哪些是公开 API
- 中断服务例程（ISR）和主循环（task）的分工
- 状态机设计（如果组件有状态）
- 错误恢复策略（通信超时、硬件异常等）

### 第 5 步：实现 Component

**产出物**：`{name}.c` 完整实现 + 更新后的头文件

实现要求：
- 严格按头文件 API 实现，不私下新增公开函数
- 寄存器操作必须对照手册注释（注明手册页码 + 寄存器名）
- 每个 `static` 函数要有用途注释
- I2C/SPI 读写在重试失败后要有超时处理和 `ESP_LOGE`
- 数据缓冲区大小用 `#define`，不 hardcode

### 第 6 步：独立 Demo

**产出物**：`Firmware/esp32_s3/demo_{name}/` 完整工程

Demo 要求：
- 是一个独立可编译的 ESP-IDF 应用工程
- 只依赖该组件 + 硬件最小系统（BSP 初始化）
- 不在 `components/` 目录下（Demo 不是组件）
- `main/demo_main.c` 只做三件事：
  1. 初始化硬件最小系统（I2C bus / GPIO）
  2. 调用组件的 init / 核心 API
  3. 打印结果到串口（`ESP_LOGI`）

Demo 示例框架：
```c
// demo_main.c — 仅演示组件用法，不包含任何业务逻辑
#include "my_sensor.h"
#include "bsp.h"        // 仅 i2c_master_init() / gpio_init()
#include "esp_log.h"

static const char *TAG = "demo_my_sensor";

void app_main(void) {
    ESP_LOGI(TAG, "=== Demo: my_sensor ===");
    bsp_i2c_init();  // 最小硬件初始化

    my_sensor_config_t cfg = MY_SENSOR_DEFAULT_CONFIG();
    ESP_ERROR_CHECK(my_sensor_init(&cfg));

    while (1) {
        if (my_sensor_is_ready()) {
            float data[3];
            my_sensor_read(data);
            ESP_LOGI(TAG, "x=%.3f y=%.3f z=%.3f", data[0], data[1], data[2]);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}
```

### 第 7 步：Demo 验证

**产出物**：串口日志输出（截图或文本）

验证清单：
- [ ] 编译 0 error 0 warning
- [ ] 烧录成功
- [ ] 组件初始化成功（日志 `xxx_init OK`）
- [ ] 核心功能正常（数据读取 / 动作执行）
- [ ] 异常情况处理（通信超时时不 crash）
- [ ] 反复 init/deinit 不内存泄漏（可选，复杂组件必须）
- [ ] 无 `abort()` / `Guru Meditation` / `panic`

### 第 8 步：测试报告

**产出物**：`test_report_{name}.md`（写在 Demo 工程目录下）

报告模板：
```markdown
# 测试报告 — {component_name}

## 测试环境
- 硬件：立创实战派 ESP32-S3
- ESP-IDF：v5.5.4
- 日期：YYYY-MM-DD

## 测试用例

| 编号 | 测试项 | 预期 | 实际 | 状态 |
|------|-------|------|------|------|
| T01 | 初始化 | ESP_OK | ESP_OK | PASS |
| T02 | 读取数据 | 有效值范围 | xxx | PASS |
| T03 | 通信超时 | ESP_ERR_TIMEOUT + 不 crash | ESP_ERR_TIMEOUT | PASS |

## 结论
所有测试用例通过，组件可用于集成。
```

## 六、工程结构

```
项目根/
├── Firmware/esp32_s3/
│   ├── {app_name}/                      ← 主应用工程（业务代码在这里）
│   │   ├── main/
│   │   │   ├── CMakeLists.txt           ← REQUIRES 引用组件
│   │   │   ├── main.c                   ← 业务主入口
│   │   │   ├── bsp.c / bsp.h            ← 硬件最小系统初始化
│   │   │   └── app_xxx.c                ← 业务逻辑/UI
│   │   └── components/
│   │       └── {component_name}/        ← 可复用组件（纯硬件驱动/协议栈）
│   │           ├── CMakeLists.txt       ← REQUIRES 显式声明依赖
│   │           ├── idf_component.yml
│   │           ├── include/
│   │           │   └── {name}.h         ← 公开 API 头文件
│   │           └── {name}.c             ← 实现（含 static 内部函数）
│   │
│   └── demo_{component_name}/           ← 独立 Demo 工程
│       ├── CMakeLists.txt
│       ├── sdkconfig.defaults
│       ├── partitions.csv
│       └── main/
│           ├── CMakeLists.txt
│           ├── demo_main.c
│           └── test_report_{name}.md    ← 测试报告在这里
│
└── Skill/Esp-Component-Skill/           ← 本 skill
    ├── SKILL.md
    ├── scripts/scaffold-component.ps1
    └── templates/
        ├── component_CMakeLists.txt
        ├── component_idf_component.yml
        ├── component_header.h
        ├── component_coding_style.md    ← 统一代码风格
        ├── component_stub.c
        ├── demo_CMakeLists.txt
        ├── demo_main_CMakeLists.txt
        ├── demo_main.c
        └── test_report_template.md
```

## 七、一键创建骨架

```powershell
& "{{PROJECT_ROOT}}/Skill/Esp-Component-Skill/scripts/scaffold-component.ps1" `
    -ProjectDir "{{PROJECT_ROOT}}" `
    -AppName "keyboard_demo" `
    -ComponentName "my_sensor" `
    -WithDemo
```

脚本生成内容：

| 生成物 | 说明 |
|--------|------|
| `components/{name}/CMakeLists.txt` | 含 REQUIRES 占位符 |
| `components/{name}/idf_component.yml` | 第三方依赖（默认仅 `idf: "^5.0"`） |
| `components/{name}/include/{name}.h` | 公开 API 头文件骨架 |
| `components/{name}/{name}.c` | stub 实现 |
| `demo_{name}/` 全套工程（若 `-WithDemo`） | 独立 Demo 工程 |

生成后需**手动**：
1. 在 `main/CMakeLists.txt` 的 `REQUIRES` 中添加组件名
2. 在 `components/{name}/CMakeLists.txt` 的 `REQUIRES` 中填写实际依赖
3. 按 8 步流程完成开发

## 八、编译 / 烧录 / 监控

**不允许内联编译命令**，统一委托给 ESP-BUILD-SKILL（`{{PROJECT_ROOT}}/Skill/Esp-Build-Skill/`）：

```powershell
# ===== 编译 =====
# 编译主应用
& "{{PROJECT_ROOT}}/Skill/Esp-Build-Skill/scripts/build.ps1" -ProjectDir "{{PROJECT_ROOT}}" -AppName "keyboard_demo" -Log
# 编译 Demo（单独验证组件）
& "{{PROJECT_ROOT}}/Skill/Esp-Build-Skill/scripts/build.ps1" -ProjectDir "{{PROJECT_ROOT}}" -AppName "demo_my_sensor" -Log

# 修改 sdkconfig 后加 -Reconfigure
& ".../build.ps1" -ProjectDir "{{PROJECT_ROOT}}" -AppName "keyboard_demo" -Reconfigure -Log

# ===== 烧录 =====
& "{{PROJECT_ROOT}}/Skill/Esp-Build-Skill/scripts/flash.ps1" -ProjectDir "{{PROJECT_ROOT}}" -AppName "demo_my_sensor" -ComPort "COM8"

# ===== 串口监控 =====
# Windows:
python "{{PROJECT_ROOT}}/Skill/Esp-Build-Skill/scripts/monitor.py" COM8 15
# Linux:
python3 "{{PROJECT_ROOT}}/Skill/Esp-Build-Skill/scripts/monitor.py" /dev/ttyUSB0 15
```

## 九、验证清单

组件开发完成后，逐项检查：

| # | 检查项 | 判定标准 |
|---|--------|----------|
| 1 | 编译 0 error 0 warning | ninja 退出码 0 |
| 2 | Demo 独立编译通过 | 不需要主应用就能编译 |
| 3 | 烧录成功 | `Hash of data verified` |
| 4 | 组件初始化成功 | 无 `abort()`，日志显示 init OK |
| 5 | 核心功能正常 | Demo 日志输出符���预期 |
| 6 | 异常不崩溃 | 拔掉传感器 / 发错误命令 → 报错但不 panic |
| 7 | 无禁区违规 | grep 确认组件内无业务/UI/AI/Demo 代码 |
| 8 | 测试报告完整 | `test_report_{name}.md` 存在且填写完整 |
| 9 | Coding Style 合规 | 对照 `component_coding_style.md` 检查 |
| 10 | 寄存器注释正确 | 每个寄存器写操作都有手册页码注释 |

## 十、常见坑点

编译错误、链接错误、I2C 限制、components 全编译问题等实战坑点，已集中记录在：

**`{{PROJECT_ROOT}}/Doc/ESP-IDF-Pitfalls.md`**

遇到编译或运行时错误时优先查这个文档。

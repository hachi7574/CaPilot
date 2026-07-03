# ESP-IDF v5.5.4 环境变量设置（每次新 PowerShell 窗口必须先 source 这个文件）
# 用法：. .\setup-env.ps1

$env:IDF_PATH = "A:\ESPIDF\Espressif5\frameworks\esp-idf-v5.5.4"
$env:IDF_PYTHON_ENV_PATH = "A:\ESPIDF\Espressif5\python_env\idf5.5_py3.11_env"
$base = "A:\ESPIDF\Espressif5\tools"
$env:PATH = "$base\cmake\3.30.2\bin;$base\ninja\1.12.1;$base\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin;$base\ccache\4.11.1;$env:IDF_PYTHON_ENV_PATH\Scripts;$env:PATH"
$env:ESP_ROM_ELF_DIR = "$base\esp-rom-elfs\20250120"

# 暴露常用工具路径变量
$global:ESP_CMAKE = "$base\cmake\3.30.2\bin\cmake.exe"
$global:ESP_NINJA = "$base\ninja\1.12.1\ninja.exe"
$global:ESP_PYTHON = "$env:IDF_PYTHON_ENV_PATH\Scripts\python.exe"
$global:ESP_BASE = $base

Write-Host "[ESP-IDF] env ready: IDF_PATH=$env:IDF_PATH" -ForegroundColor Green

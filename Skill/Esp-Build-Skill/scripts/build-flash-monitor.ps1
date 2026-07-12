# ESP-IDF build + flash + monitor (all-in-one)
# Usage:
#   powershell -ExecutionPolicy Bypass -File build-flash-monitor.ps1 -ProjectDir "B:\CaPilot" -AppName "keyboard_demo" -ComPort "COM8"
#   powershell -ExecutionPolicy Bypass -File build-flash-monitor.ps1 -ProjectDir "B:\CaPilot" -AppName "keyboard_demo" -ComPort "COM8" -MonitorSec 15 -Reconfigure

param(
    [Parameter(Mandatory=$true)][string]$ProjectDir,
    [Parameter(Mandatory=$true)][string]$AppName,
    [string]$ComPort = "AUTO",
    [int]$MonitorSec = 0,        # 0 = infinite until Ctrl+C; >0 = exit after N seconds
    [switch]$Reconfigure,
    [switch]$WithStorage,
    [switch]$SkipMonitor
)

# 1. Build
Write-Host "=== STEP 1/3: BUILD ===" -ForegroundColor Yellow
& "$PSScriptRoot\build.ps1" -ProjectDir $ProjectDir -AppName $AppName -Reconfigure:$Reconfigure -Log
if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED, abort." -ForegroundColor Red; exit 1 }

# 2. Flash
Write-Host "=== STEP 2/3: FLASH ===" -ForegroundColor Yellow
& "$PSScriptRoot\flash.ps1" -ProjectDir $ProjectDir -AppName $AppName -ComPort $ComPort -WithStorage:$WithStorage
if ($LASTEXITCODE -ne 0) { Write-Host "FLASH FAILED, abort." -ForegroundColor Red; exit 1 }

# 3. Monitor
if ($SkipMonitor) {
    Write-Host "=== STEP 3/3: MONITOR (skipped) ===" -ForegroundColor Yellow
    exit 0
}

Write-Host "=== STEP 3/3: MONITOR ===" -ForegroundColor Yellow
. "$PSScriptRoot\setup-env.ps1"

# Use python monitor.py (avoids esp_idf_monitor Start-Job security limit)
$monitorScript = Join-Path $PSScriptRoot "monitor.py"
if ($MonitorSec -gt 0) {
    & $ESP_PYTHON $monitorScript $ComPort $MonitorSec
} else {
    & $ESP_PYTHON $monitorScript $ComPort
}

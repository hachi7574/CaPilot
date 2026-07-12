# ESP-IDF flash script (esptool)
# Usage:
#   powershell -ExecutionPolicy Bypass -File flash.ps1 -ProjectDir "B:\CaPilot" -AppName "keyboard_demo" -ComPort "COM8"
#   powershell -ExecutionPolicy Bypass -File flash.ps1 -ProjectDir "B:\CaPilot" -AppName "keyboard_demo" -ComPort "AUTO"
#   powershell -ExecutionPolicy Bypass -File flash.ps1 -ProjectDir "B:\CaPilot" -AppName "keyboard_demo" -ComPort "COM8" -WithStorage

param(
    [Parameter(Mandatory=$true)][string]$ProjectDir,
    [Parameter(Mandatory=$true)][string]$AppName,
    [string]$ComPort = "AUTO",
    [switch]$WithStorage   # also flash storage.bin (SPIFFS partition)
)

. "$PSScriptRoot\setup-env.ps1"

$legacyAppPath = Join-Path $ProjectDir "firmware\esp32_s3\$AppName"
$componentDemoPath = Join-Path $ProjectDir "Firmware\Components_Demos\$AppName"

if (Test-Path $legacyAppPath) {
    $appPath = $legacyAppPath
} elseif (Test-Path $componentDemoPath) {
    $appPath = $componentDemoPath
} else {
    Write-Host "[FLASH] App not found: $AppName" -ForegroundColor Red
    Write-Host "  Checked: $legacyAppPath"
    Write-Host "  Checked: $componentDemoPath"
    exit 1
}

# Auto-detect COM port (pick highest COM number)
if ($ComPort -eq "AUTO") {
    $ports = [System.IO.Ports.SerialPort]::GetPortNames() | Where-Object { $_ -match "COM\d+" }
    if ($ports.Count -eq 0) {
        Write-Host "[FLASH] No COM port found" -ForegroundColor Red; exit 1
    }
    $ComPort = ($ports | Sort-Object { [int]($_ -replace '\D','') } -Descending)[0]
    Write-Host "[FLASH] Auto-detected: $ComPort" -ForegroundColor Yellow
}

Write-Host "[FLASH] Port=$ComPort App=$AppName" -ForegroundColor Cyan
Set-Location $appPath

$flashArgs = @(
    "-m", "esptool", "--chip", "esp32s3", "-p", $ComPort, "-b", "460800",
    "--before", "default_reset", "--after", "hard_reset",
    "write_flash", "--flash_mode", "dio", "--flash_size", "16MB", "--flash_freq", "80m",
    "0x0", "build/bootloader/bootloader.bin",
    "0x8000", "build/partition_table/partition-table.bin",
    "0x10000", "build/$AppName.bin"
)
if ($WithStorage) {
    $flashArgs += @("0x310000", "build/storage.bin")
}

& $ESP_PYTHON @flashArgs
$exitCode = $LASTEXITCODE

if ($exitCode -eq 0) {
    Write-Host "[FLASH] OK" -ForegroundColor Green
} else {
    Write-Host "[FLASH] FAILED (exit $exitCode)" -ForegroundColor Red
    Write-Host "Tip: close serial monitor windows and retry"
}
exit $exitCode

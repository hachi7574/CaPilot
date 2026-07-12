# ESP-IDF build script (cmake + ninja, recommended for sandbox env)
# Usage:
#   powershell -ExecutionPolicy Bypass -File build.ps1 -ProjectDir "B:\CaPilot" -AppName "keyboard_demo"
#   powershell -ExecutionPolicy Bypass -File build.ps1 -ProjectDir "B:\CaPilot" -AppName "keyboard_demo" -Reconfigure
#   powershell -ExecutionPolicy Bypass -File build.ps1 -ProjectDir "B:\CaPilot" -AppName "keyboard_demo" -Log

param(
    [Parameter(Mandatory=$true)][string]$ProjectDir,
    [Parameter(Mandatory=$true)][string]$AppName,
    [switch]$Reconfigure,    # delete build/sdkconfig/managed_components then reconfigure
    [switch]$Log             # save log to logs/ dir
)

. "$PSScriptRoot\setup-env.ps1"

$legacyAppPath = Join-Path $ProjectDir "firmware\esp32_s3\$AppName"
$componentDemoPath = Join-Path $ProjectDir "Firmware\Components_Demos\$AppName"

if (Test-Path $legacyAppPath) {
    $appPath = $legacyAppPath
} elseif (Test-Path $componentDemoPath) {
    $appPath = $componentDemoPath
} else {
    Write-Host "[BUILD] App not found: $AppName" -ForegroundColor Red
    Write-Host "  Checked: $legacyAppPath"
    Write-Host "  Checked: $componentDemoPath"
    exit 1
}
$buildPath = Join-Path $appPath "build"

if ($Reconfigure) {
    Write-Host "[BUILD] Reconfigure: cleaning build/sdkconfig/managed_components" -ForegroundColor Yellow
    if (Test-Path $buildPath) { Remove-Item -Recurse -Force $buildPath }
    if (Test-Path (Join-Path $appPath "sdkconfig")) { Remove-Item -Force (Join-Path $appPath "sdkconfig") }
    if (Test-Path (Join-Path $appPath "managed_components")) { Remove-Item -Recurse -Force (Join-Path $appPath "managed_components") }
}

# CMake configure (first time or after CMakeLists change)
if (-not (Test-Path (Join-Path $buildPath "build.ninja"))) {
    Write-Host "[BUILD] CMake configure..." -ForegroundColor Cyan
    & $ESP_CMAKE -G Ninja -DPYTHON_DEPS_CHECKED=1 `
        "-DPYTHON=$ESP_PYTHON" -DESP_PLATFORM=1 `
        "-DCMAKE_MAKE_PROGRAM=$ESP_NINJA" -DCCACHE_ENABLE=0 `
        -B $buildPath -S $appPath
    if ($LASTEXITCODE -ne 0) { Write-Host "[BUILD] CMake config FAILED" -ForegroundColor Red; exit 1 }
}

# Ninja build
Write-Host "[BUILD] ninja..." -ForegroundColor Cyan
Set-Location $buildPath

if ($Log) {
    $logsDir = Join-Path $appPath "logs"
    if (-not (Test-Path $logsDir)) { New-Item -ItemType Directory -Path $logsDir | Out-Null }
    $logFile = Join-Path $logsDir "build_$(Get-Date -Format yyyyMMdd_HHmmss).log"
    & $ESP_NINJA 2>&1 | Out-File -FilePath $logFile -Encoding utf8
    $exitCode = $LASTEXITCODE
    if ($exitCode -ne 0) {
        Write-Host "[BUILD] FAILED. Errors:" -ForegroundColor Red
        Select-String -Path $logFile -Pattern "FAILED|fatal error|error:" | Select-Object -First 10
    } else {
        Write-Host "[BUILD] OK. Log: $logFile" -ForegroundColor Green
    }
    exit $exitCode
} else {
    & $ESP_NINJA
    exit $LASTEXITCODE
}

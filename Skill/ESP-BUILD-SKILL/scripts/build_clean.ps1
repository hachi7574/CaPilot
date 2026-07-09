$env:IDF_PATH = "A:\ESPIDF\Espressif5\frameworks\esp-idf-v5.5.4"
$env:IDF_PYTHON_ENV_PATH = "A:\ESPIDF\Espressif5\python_env\idf5.5_py3.11_env"
$base = "A:\ESPIDF\Espressif5\tools"
$env:PATH = "$base\cmake\3.30.2\bin;$base\ninja\1.12.1;$base\xtensa-esp-elf\esp-14.2.0_20260121\xtensa-esp-elf\bin;$base\ccache\4.11.1;$env:IDF_PYTHON_ENV_PATH\Scripts;$env:PATH"
$env:ESP_ROM_ELF_DIR = "$base\esp-rom-elfs\20250120"

$cmake = "$base\cmake\3.30.2\bin\cmake.exe"
$ninja = "$base\ninja\1.12.1\ninja.exe"
$python = "$env:IDF_PYTHON_ENV_PATH\Scripts\python.exe"
$appPath = "B:\CaPilot\firmware\esp32_s3\imu_demo"

if (Test-Path "$appPath\build") { Remove-Item -Recurse -Force "$appPath\build" }
New-Item -ItemType Directory -Force -Path "$appPath\build" | Out-Null

Write-Host "[CMAKE] Configuring..." -ForegroundColor Cyan

# Run cmake, redirect stderr to a file to avoid PowerShell native error mangling
$proc = Start-Process -FilePath $cmake -ArgumentList @(
    "-G", "Ninja",
    "-DPYTHON_DEPS_CHECKED=1",
    "-DPYTHON=$python",
    "-DESP_PLATFORM=1",
    "-DIDF_TARGET=esp32s3",
    "-DCMAKE_MAKE_PROGRAM=$ninja",
    "-DCCACHE_ENABLE=0",
    "-B", "$appPath\build",
    "-S", $appPath
) -Wait -NoNewWindow -RedirectStandardError "$appPath\logs\cmake_err.log" -RedirectStandardOutput "$appPath\logs\cmake_out.log" -PassThru

$exitCode = $proc.ExitCode
if ($exitCode -ne 0) {
    Write-Host "CMake FAILED (exit=$exitCode)" -ForegroundColor Red
    Get-Content "$appPath\logs\cmake_err.log" | Select-String -Pattern "error" -CaseSensitive:$false | Select-Object -First 30
    exit $exitCode
}

Write-Host "CMake OK, running ninja..." -ForegroundColor Green
Set-Location "$appPath\build"
Start-Process -FilePath $ninja -Wait -NoNewWindow -RedirectStandardError "$appPath\logs\ninja_err.log" -RedirectStandardOutput "$appPath\logs\ninja_out.log" -PassThru
$ninjaExit = $LASTEXITCODE
if ($ninjaExit -ne 0) {
    Write-Host "NINJA FAILED" -ForegroundColor Red
    Get-Content "$appPath\logs\ninja_err.log" | Select-Object -First 30
} else {
    Write-Host "BUILD OK" -ForegroundColor Green
    Select-String -Path "$appPath\logs\ninja_out.log" -Pattern "imu_demo.bin|Generated" | Select-Object -First 5
}
exit $ninjaExit

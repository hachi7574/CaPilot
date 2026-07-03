# scaffold-component.ps1
# Generate ESP-IDF component skeleton in one shot.
# Usage:
#   powershell -ExecutionPolicy Bypass -File scaffold-component.ps1 `
#       -ProjectDir "B:\CaPilot" -AppName "keyboard_demo" -ComponentName "my_module"
#
# Output:
#   firmware/esp32_s3/{AppName}/components/{ComponentName}/
#     CMakeLists.txt
#     idf_component.yml
#     include/{ComponentName}.h
#     {ComponentName}.c   (stub)

param(
    [Parameter(Mandatory=$true)]
    [string]$ProjectDir,

    [Parameter(Mandatory=$true)]
    [string]$AppName,

    [Parameter(Mandatory=$true)]
    [string]$ComponentName
)

$ErrorActionPreference = "Stop"

$SkillRoot     = "B:\CaPilot\Skill\ESP-COMPONENT-SKILL"
$TemplateDir   = Join-Path $SkillRoot "templates"
$AppDir        = Join-Path $ProjectDir "firmware\esp32_s3\$AppName"
$CompDir       = Join-Path $AppDir "components\$ComponentName"
$IncludeDir    = Join-Path $CompDir "include"

if (-not (Test-Path $AppDir)) {
    Write-Error "App dir not found: $AppDir"
    exit 1
}
if (Test-Path $CompDir) {
    Write-Error "Component already exists: $CompDir"
    exit 1
}

$UpperName = $ComponentName.ToUpper()

New-Item -ItemType Directory -Path $IncludeDir -Force | Out-Null
Write-Host "[1/5] Created: $CompDir"

# 1. CMakeLists.txt
$cmakeContent = Get-Content (Join-Path $TemplateDir "component_CMakeLists.txt") -Raw
$cmakeContent = $cmakeContent -replace '\{\{COMPONENT_NAME\}\}', $ComponentName
Set-Content -Path (Join-Path $CompDir "CMakeLists.txt") -Value $cmakeContent -Encoding utf8
Write-Host "[2/5] CMakeLists.txt"

# 2. idf_component.yml
Copy-Item (Join-Path $TemplateDir "component_idf_component.yml") -Destination (Join-Path $CompDir "idf_component.yml")
Write-Host "[3/5] idf_component.yml"

# 3. Header
$headerContent = Get-Content (Join-Path $TemplateDir "component_header.h") -Raw
$headerContent = $headerContent -replace '\{\{COMPONENT_NAME_UPPER\}\}', $UpperName
$headerContent = $headerContent -replace '\{\{component_name\}\}', $ComponentName
Set-Content -Path (Join-Path $IncludeDir "$ComponentName.h") -Value $headerContent -Encoding utf8
Write-Host "[4/5] include/$ComponentName.h"

# 4. Source stub (use single-quoted here-string, no variable expansion issues)
$stub = @'
#include "__COMP__.h"
#include "esp_log.h"

static const char *TAG = "__COMP__";

esp_err_t __COMP___init(void) {
    ESP_LOGI(TAG, "init");
    return ESP_OK;
}

bool __COMP___is_ready(void) {
    return true;
}
'@
$stub = $stub -replace '__COMP__', $ComponentName
Set-Content -Path (Join-Path $CompDir "$ComponentName.c") -Value $stub -Encoding utf8
Write-Host "[5/5] $ComponentName.c"

Write-Host ""
Write-Host "DONE: $CompDir" -ForegroundColor Green
Write-Host ""
Write-Host "Next steps:"
Write-Host "  1. Implement logic in $ComponentName.c"
Write-Host "  2. Add '$ComponentName' to main/CMakeLists.txt REQUIRES"
Write-Host "  3. Build with ESP-BUILD-SKILL:"
Write-Host "     powershell -ExecutionPolicy Bypass -File B:\CaPilot\Skill\ESP-BUILD-SKILL\scripts\build.ps1 -ProjectDir $ProjectDir -AppName $AppName -Reconfigure -Log"

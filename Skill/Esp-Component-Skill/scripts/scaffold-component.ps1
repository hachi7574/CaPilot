# scaffold-component.ps1
# Generate ESP-IDF reusable component skeleton.
#
# Usage:
#   .\scaffold-component.ps1 -ProjectDir "." -AppName "keyboard_demo" -ComponentName "my_sensor"
#   .\scaffold-component.ps1 -ProjectDir "." -AppName "keyboard_demo" -ComponentName "my_sensor" -WithDemo
#
# Output:
#   Firmware/esp32_s3/{AppName}/components/{ComponentName}/
#     CMakeLists.txt
#     idf_component.yml
#     include/{ComponentName}.h
#     {ComponentName}.c   (stub)
#
#   (if -WithDemo)
#   Firmware/esp32_s3/demo_{ComponentName}/
#     CMakeLists.txt
#     sdkconfig.defaults
#     partitions.csv
#     main/
#       CMakeLists.txt
#       demo_main.c

param(
    [Parameter(Mandatory=$true)]
    [string]$ProjectDir,

    [Parameter(Mandatory=$true)]
    [string]$AppName,

    [Parameter(Mandatory=$true)]
    [string]$ComponentName,

    [switch]$WithDemo = $false
)

$ErrorActionPreference = "Stop"

# Paths (relative to project root)
$scriptDir     = Split-Path -Parent $MyInvocation.MyCommand.Path
$SkillRoot     = Split-Path -Parent $scriptDir   # ../  -> Skill/Esp-Component-Skill/
$TemplateDir   = Join-Path $SkillRoot "templates"
$AppDir        = Join-Path $ProjectDir "Firmware\esp32_s3\$AppName"
$CompDir       = Join-Path $AppDir "components\$ComponentName"
$IncludeDir    = Join-Path $CompDir "include"
$DemoDir       = Join-Path $ProjectDir "Firmware\esp32_s3\demo_$ComponentName"

$UpperName = $ComponentName.ToUpper()

# ========== Validate ==========
if (-not (Test-Path $AppDir)) {
    Write-Error "App dir not found: $AppDir"
    Write-Host "Tip: Create the app project first (CMakeLists.txt + main/)"
    exit 1
}
if (Test-Path $CompDir) {
    Write-Error "Component already exists: $CompDir"
    exit 1
}
if ($WithDemo -and (Test-Path $DemoDir)) {
    Write-Error "Demo already exists: $DemoDir"
    exit 1
}

# ========== Component Skeleton ==========
Write-Host ""
Write-Host "=== Creating component: $ComponentName ===" -ForegroundColor Cyan

New-Item -ItemType Directory -Path $IncludeDir -Force | Out-Null
Write-Host "[1/4] Created: $CompDir"

# 1. CMakeLists.txt
$cmakeContent = Get-Content (Join-Path $TemplateDir "component_CMakeLists.txt") -Raw
$cmakeContent = $cmakeContent -replace '\{\{COMPONENT_NAME\}\}', $ComponentName
Set-Content -Path (Join-Path $CompDir "CMakeLists.txt") -Value $cmakeContent -Encoding utf8
Write-Host "[2/4] CMakeLists.txt"

# 2. idf_component.yml
Copy-Item (Join-Path $TemplateDir "component_idf_component.yml") `
    -Destination (Join-Path $CompDir "idf_component.yml")
Write-Host "[3/4] idf_component.yml"

# 3. Header (uses new config_t + DEFAULT_CONFIG pattern)
$headerContent = Get-Content (Join-Path $TemplateDir "component_header.h") -Raw
$headerContent = $headerContent -replace '\{\{COMPONENT_NAME_UPPER\}\}', $UpperName
$headerContent = $headerContent -replace '\{\{component_name\}\}', $ComponentName
Set-Content -Path (Join-Path $IncludeDir "$ComponentName.h") -Value $headerContent -Encoding utf8
Write-Host "[4/4] include/$ComponentName.h"

# 4. Source stub (from template, not inline)
$stubContent = Get-Content (Join-Path $TemplateDir "component_stub.c") -Raw
$stubContent = $stubContent -replace '\{\{COMPONENT_NAME\}\}', $ComponentName
$stubContent = $stubContent -replace '\{\{COMPONENT_NAME_UPPER\}\}', $UpperName
Set-Content -Path (Join-Path $CompDir "$ComponentName.c") -Value $stubContent -Encoding utf8
Write-Host "[4/4] $ComponentName.c"

Write-Host ""
Write-Host "Component skeleton ready: $CompDir" -ForegroundColor Green

# ========== Demo Project (optional) ==========
if ($WithDemo) {
    Write-Host ""
    Write-Host "=== Creating demo: demo_$ComponentName ===" -ForegroundColor Cyan

    $DemoMainDir = Join-Path $DemoDir "main"
    New-Item -ItemType Directory -Path $DemoMainDir -Force | Out-Null
    Write-Host "[1/6] Created: $DemoDir"

    # Demo CMakeLists.txt
    $demoCmakeContent = Get-Content (Join-Path $TemplateDir "demo_CMakeLists.txt") -Raw
    $demoCmakeContent = $demoCmakeContent -replace '\{\{COMPONENT_NAME\}\}', $ComponentName
    $demoCmakeContent = $demoCmakeContent -replace '\{\{APP_NAME\}\}', $AppName
    Set-Content -Path (Join-Path $DemoDir "CMakeLists.txt") -Value $demoCmakeContent -Encoding utf8
    Write-Host "[2/6] CMakeLists.txt"

    # sdkconfig.defaults (copy from existing template)
    Copy-Item (Join-Path $TemplateDir "sdkconfig.defaults") `
        -Destination (Join-Path $DemoDir "sdkconfig.defaults")
    Write-Host "[3/6] sdkconfig.defaults"

    # partitions.csv (copy from existing template)
    Copy-Item (Join-Path $TemplateDir "partitions.csv") `
        -Destination (Join-Path $DemoDir "partitions.csv")
    Write-Host "[4/6] partitions.csv"

    # main/CMakeLists.txt
    $demoMainCmakeContent = Get-Content (Join-Path $TemplateDir "demo_main_CMakeLists.txt") -Raw
    $demoMainCmakeContent = $demoMainCmakeContent -replace '\{\{COMPONENT_NAME\}\}', $ComponentName
    $demoMainCmakeContent = $demoMainCmakeContent -replace '\{\{APP_NAME\}\}', $AppName
    Set-Content -Path (Join-Path $DemoMainDir "CMakeLists.txt") -Value $demoMainCmakeContent -Encoding utf8
    Write-Host "[5/6] main/CMakeLists.txt"

    # demo_main.c
    $demoMainContent = Get-Content (Join-Path $TemplateDir "demo_main.c") -Raw
    $demoMainContent = $demoMainContent -replace '\{\{COMPONENT_NAME\}\}', $ComponentName
    $demoMainContent = $demoMainContent -replace '\{\{COMPONENT_NAME_UPPER\}\}', $UpperName
    Set-Content -Path (Join-Path $DemoMainDir "demo_main.c") -Value $demoMainContent -Encoding utf8
    Write-Host "[6/6] main/demo_main.c"

    # test report template
    $reportContent = Get-Content (Join-Path $TemplateDir "test_report_template.md") -Raw
    $reportContent = $reportContent -replace '\{\{COMPONENT_NAME\}\}', $ComponentName
    Set-Content -Path (Join-Path $DemoMainDir "test_report_$ComponentName.md") -Value $reportContent -Encoding utf8

    Write-Host ""
    Write-Host "Demo project ready: $DemoDir" -ForegroundColor Green
}

# ========== Next Steps ==========
Write-Host ""
Write-Host "Next steps:" -ForegroundColor Yellow
Write-Host "  1. Check chip manual (if driving a peripheral):"
Write-Host "     Skill/Esp-Datasheet-Skill/scripts/convert_pdf.py search-md <chip> <keyword>"
Write-Host ""
Write-Host "  2. Follow the 8-step workflow in SKILL.md:"
Write-Host "     [1] Requirement → [2] Architecture → [3] API Design"
Write-Host "     [4] Internal Design → [5] Implement → [6] Demo"
Write-Host "     [7] Verify → [8] Test Report"
Write-Host ""
Write-Host "  3. Edit CMakeLists.txt REQUIRES with actual dependencies"
Write-Host "  4. Add '$ComponentName' to main/CMakeLists.txt REQUIRES"
Write-Host "  5. Review coding style: templates/component_coding_style.md"
if ($WithDemo) {
    Write-Host "  6. Build demo:"
    Write-Host "     Esp-Build-Skill/scripts/build.ps1 -ProjectDir ""$ProjectDir"" -AppName ""demo_$ComponentName"" -Log"
}

# ============================================================
# setup-skill-links.ps1
# Windows PowerShell Skill Link Script
#
# Usage:
#   powershell -ExecutionPolicy Bypass -File Skill\Skill-Sync-Scripts\setup-skill-links.ps1
#   powershell -ExecutionPolicy Bypass -File Skill\Skill-Sync-Scripts\setup-skill-links.ps1 -Clean
#
# Principle: Use Python subprocess to create Junctions (mklink /J)
# ============================================================

param([switch]$Clean)

$ErrorActionPreference = "Stop"

# --- Locate project root ---
# Script is at Skill\Skill-Sync-Scripts\, go up two levels
$ScriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $ScriptDir)
$SkillSource = Join-Path $ProjectRoot "Skill"

# --- Config: supported agent skill dirs ---
$AgentDirs = @(
    ".claude\skills",
    ".codebuddy\skills",
    ".agents\skills",
    ".cursor\skills",
    ".windsurf\skills"
)

# --- Find Python ---
$PythonBin = (Get-Command python -ErrorAction SilentlyContinue).Source
if (-not $PythonBin) {
    $PythonBin = (Get-Command python3 -ErrorAction SilentlyContinue).Source
}
if (-not $PythonBin) {
    Write-Host "[ERROR] Python not found" -ForegroundColor Red
    exit 1
}

# --- Helper: create Junction via Python subprocess ---
function New-Junction {
    param([string]$LinkPath, [string]$TargetPath)

    $pyScript = @"
import subprocess, sys, os
result = subprocess.run(
    ['cmd', '/c', 'mklink', '/J', sys.argv[1], sys.argv[2]],
    capture_output=True
)
if result.returncode != 0 or not os.path.exists(sys.argv[1]):
    print(result.stderr.decode('gbk', errors='replace'), file=sys.stderr)
    sys.exit(1)
"@

    $result = & $PythonBin -c $pyScript $LinkPath $TargetPath 2>&1
    return $LASTEXITCODE -eq 0
}

# --- Helper: remove Junction ---
function Remove-Junction {
    param([string]$Path)
    & $PythonBin -c "import os, sys; os.rmdir(sys.argv[1])" $Path 2>$null
    return $LASTEXITCODE -eq 0
}

# --- Clean mode ---
if ($Clean) {
    Write-Host "=== Clean all agent skill links ===" -ForegroundColor Yellow
    foreach ($agentDir in $AgentDirs) {
        $fullDir = Join-Path $ProjectRoot $agentDir
        if (-not (Test-Path $fullDir)) { continue }

        Get-ChildItem $fullDir -Force | ForEach-Object {
            $item = $_
            # Check if it's a Junction or SymbolicLink
            if ($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
                if (Remove-Junction $item.FullName) {
                    Write-Host "  [REMOVED] $($item.Name)" -ForegroundColor Green
                }
            }
        }
    }
    Write-Host "Done. Skill/ source files not affected." -ForegroundColor Green
    exit 0
}

# --- Main logic ---
Write-Host "=== setup-skill-links ===" -ForegroundColor Cyan
Write-Host "Project root: $ProjectRoot"
Write-Host "Skill source: $SkillSource"
Write-Host "Python: $PythonBin"
Write-Host ""

# Check Skill/ exists
if (-not (Test-Path $SkillSource)) {
    Write-Host "[ERROR] Skill/ not found: $SkillSource" -ForegroundColor Red
    exit 1
}

# List skills (exclude Skill-Sync-Scripts)
$SkillDirs = Get-ChildItem $SkillSource -Directory | Where-Object { $_.Name -ne "Skill-Sync-Scripts" } | Select-Object -ExpandProperty Name
if ($SkillDirs.Count -eq 0) {
    Write-Host "[ERROR] No skill dirs found in Skill/" -ForegroundColor Red
    exit 1
}

Write-Host "Found $($SkillDirs.Count) skills: $($SkillDirs -join ', ')"
Write-Host ""

# Create links for each agent dir
$created = 0
$failed = 0

foreach ($agentDir in $AgentDirs) {
    $fullDir = Join-Path $ProjectRoot $agentDir

    # Only process existing agent dirs
    if (-not (Test-Path $fullDir)) { continue }

    Write-Host ">> $agentDir"

    foreach ($skillName in $SkillDirs) {
        $target = Join-Path $SkillSource $skillName
        $linkPath = Join-Path $fullDir $skillName

        # Remove existing link/junction
        if (Test-Path $linkPath) {
            $item = Get-Item $linkPath -Force
            if ($item.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
                Remove-Junction $linkPath | Out-Null
            } elseif ($item.PSIsContainer) {
                # Real dir (old copy) -> backup
                $backup = "$linkPath.bak.$([DateTimeOffset]::UtcNow.ToUnixTimeSeconds())"
                Write-Host "  [BACKUP] old copy -> $backup" -ForegroundColor Yellow
                Move-Item $linkPath $backup
            } else {
                Write-Host "  [SKIP] $skillName exists but not a dir" -ForegroundColor Yellow
                $failed++
                continue
            }
        }

        # Create Junction
        if (New-Junction $linkPath $target) {
            Write-Host "  [JUNCTION] $skillName -> Skill/$skillName" -ForegroundColor Green
            $created++
        } else {
            Write-Host "  [FAIL] $skillName" -ForegroundColor Red
            $failed++
        }
    }
    Write-Host ""
}

Write-Host "=== Done ===" -ForegroundColor Cyan
Write-Host "Created: $created  Failed: $failed"
if ($failed -gt 0) {
    Write-Host "Some links failed. Check log above." -ForegroundColor Red
    exit 1
}

<#
.SYNOPSIS
  Builds full micro-ROS firmware inside WSL2 (required on Windows).
.PARAMETER Port
  Serial port for upload from Windows (default COM16).
.PARAMETER OtaIp
  If set, upload via espota to this IP after the WSL build.
.PARAMETER SkipUpload
  Build only inside WSL.
.PARAMETER ManualBootloader
  Prompt for manual bootloader before serial upload.
#>
param(
  [string]$Port = "COM16",
  [string]$OtaIp = "",
  [switch]$SkipUpload,
  [switch]$ManualBootloader
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot "Common.ps1")
. (Join-Path $PSScriptRoot "Resolve-PlatformIO.ps1")

$FirmwareDir = Join-Path $RepoRoot "firmware"
$FirmwareInclude = Join-Path $FirmwareDir "include"
[void](Set-RoseeyesSeedBuildFlags -RequireAgentIp -FirmwareIncludeDir $FirmwareInclude)

$WslFirmware = (wsl -e wslpath -a $FirmwareDir).Trim()
if (-not $WslFirmware) {
  throw "Failed to convert firmware path for WSL."
}

$ShLines = @(
  '#!/usr/bin/env bash',
  'set -euo pipefail',
  'export PATH="$HOME/.local/bin:$PATH"',
  'if ! command -v pio >/dev/null 2>&1; then',
  '  echo "Installing PlatformIO Core into WSL user site..."',
  '  python3 -m pip install --user -U platformio',
  'fi',
  "cd '$WslFirmware'",
  'echo "Building xiao_esp32s3 (micro-ROS) in WSL..."',
  'pio run -e xiao_esp32s3'
)

$ShPathWin = Join-Path $FirmwareDir ".wsl_build_microros.sh"
[System.IO.File]::WriteAllText($ShPathWin, ($ShLines -join "`n") + "`n")
$ShPathWsl = "$WslFirmware/.wsl_build_microros.sh"

Write-Host "Invoking WSL PlatformIO build (first micro-ROS compile can take 10-20+ minutes)..."
wsl -e bash $ShPathWsl
$buildExit = $LASTEXITCODE
Remove-Item $ShPathWin -ErrorAction SilentlyContinue
if ($buildExit -ne 0) { exit $buildExit }

if ($SkipUpload) {
  Write-Host "SkipUpload set - WSL build finished."
  exit 0
}

$Pio = Get-PlatformIoCommand
$uploadExit = Invoke-RoseeyesUpload `
  -Pio $Pio `
  -ProjectDir $FirmwareDir `
  -EnvName "xiao_esp32s3" `
  -Port $Port `
  -OtaIp $OtaIp `
  -ManualBootloader:$ManualBootloader

if ($uploadExit -is [array]) { $uploadExit = $uploadExit[-1] }
exit ([int]$uploadExit)

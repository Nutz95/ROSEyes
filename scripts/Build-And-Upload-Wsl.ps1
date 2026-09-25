<#
.SYNOPSIS
  Builds full micro-ROS firmware inside WSL2 (required on Windows).
.PARAMETER Port
  Serial port for upload from Windows (default COM16).
.PARAMETER OtaIp
  If set, upload via espota to this IP after the WSL build (no Windows rebuild).
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

$EnsureScriptWin = Join-Path $PSScriptRoot "Ensure-WslPlatformIo.sh"
if (-not (Test-Path $EnsureScriptWin)) {
  throw "Missing $EnsureScriptWin"
}

$WslFirmware = (wsl -e wslpath -a $FirmwareDir).Trim()
$WslEnsure = (wsl -e wslpath -a $EnsureScriptWin).Trim()
$WslRepoRoot = (wsl -e wslpath -a $RepoRoot).Trim()
if (-not $WslFirmware) {
  throw "Failed to convert firmware path for WSL."
}
if (-not $WslEnsure) {
  throw "Failed to convert Ensure-WslPlatformIo.sh path for WSL."
}
if (-not $WslRepoRoot) {
  throw "Failed to convert repo root path for WSL."
}

# Build on the Linux filesystem: ament/colcon shell hooks break with CRLF on /mnt/*.
$ShLines = @(
  '#!/usr/bin/env bash'
  'set -euo pipefail'
  'export PATH="$HOME/.platformio/penv/bin:$HOME/.local/bin:$PATH"'
  "bash '$WslEnsure'"
  'LINUX_ROOT="$HOME/roseyes-wsl-build"'
  'LINUX_FW="$LINUX_ROOT/firmware"'
  'mkdir -p "$LINUX_ROOT"'
  'echo "Syncing firmware sources to $LINUX_FW (avoid /mnt CRLF breaks)..."'
  ('rsync -a --delete --exclude ''.pio'' --exclude ''.wsl_build_microros.sh'' ''' + $WslFirmware + '/'' "$LINUX_FW/"')
  'mkdir -p "$LINUX_ROOT/resources"'
  ('rsync -a --delete ''' + $WslRepoRoot + '/resources/eyes/'' "$LINUX_ROOT/resources/eyes/"')
  'cd "$LINUX_FW"'
  'echo "Building xiao_esp32s3 (micro-ROS) in WSL Linux home..."'
  'pio run -e xiao_esp32s3'
  'echo "Copying build artifacts back to Windows tree..."'
  ("mkdir -p '" + $WslFirmware + "/.pio/build/xiao_esp32s3'")
  ('rsync -a "$LINUX_FW/.pio/build/xiao_esp32s3/" ''' + $WslFirmware + '/.pio/build/xiao_esp32s3/''')
  'test -f "$LINUX_FW/.pio/build/xiao_esp32s3/firmware.bin"'
  ('test -f ''' + $WslFirmware + '/.pio/build/xiao_esp32s3/firmware.bin''')
  'echo "firmware.bin ready (WSL + Windows tree)"'
)

$ShPathWin = Join-Path $FirmwareDir ".wsl_build_microros.sh"
$lf = ($ShLines -join "`n") + "`n"
[System.IO.File]::WriteAllText($ShPathWin, $lf)
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

$FirmwareBin = Join-Path $FirmwareDir ".pio\build\xiao_esp32s3\firmware.bin"
if (-not (Test-Path $FirmwareBin)) {
  throw "WSL build succeeded but firmware.bin missing at $FirmwareBin"
}

if ($OtaIp) {
  # Never run pio -e *_ota on Windows: it rebuilds micro-ROS and crashes.
  $uploadExit = Invoke-RoseeyesPrebuiltOtaUpload -FirmwareBin $FirmwareBin -OtaIp $OtaIp
} else {
  Write-Warning "Serial upload after WSL still uses Windows PlatformIO and may rebuild micro-ROS."
  Write-Warning "Prefer -OtaIp <device-ip> once the board is on WiFi."
  $Pio = Get-PlatformIoCommand
  $uploadExit = Invoke-RoseeyesUpload `
    -Pio $Pio `
    -ProjectDir $FirmwareDir `
    -EnvName "xiao_esp32s3" `
    -Port $Port `
    -ManualBootloader:$ManualBootloader
}

if ($uploadExit -is [array]) { $uploadExit = $uploadExit[-1] }
exit ([int]$uploadExit)

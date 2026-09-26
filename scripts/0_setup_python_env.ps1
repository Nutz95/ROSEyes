<#
.SYNOPSIS
  Activate ROS 2, upgrade pip, install python/requirements.txt (pygame, Pillow, …).
.DESCRIPTION
  Dot-sources Activate-Ros.ps1 so rclpy is available, then refreshes pip + host
  tools listed in python/requirements.txt. Also upgrades PlatformIO penv pip
  (silences the "new release of pip" spam during firmware uploads).
.PARAMETER RosSetup
  Path to ROS 2 setup.ps1. Defaults to $env:ROS2_WINDOWS_SETUP.
.PARAMETER SkipPlatformIoPip
  Do not touch ~/.platformio/penv pip.
.EXAMPLE
  .\scripts\0_setup_python_env.ps1
.EXAMPLE
  . .\scripts\0_setup_python_env.ps1   # leave ROS env active in this shell
#>
param(
  [string]$RosSetup = $env:ROS2_WINDOWS_SETUP,
  [switch]$SkipPlatformIoPip
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$PythonDir = Join-Path $RepoRoot "python"
$Requirements = Join-Path $PythonDir "requirements.txt"
. (Join-Path $PSScriptRoot "Common.ps1")

Set-RoseeyesPythonIoUtf8
Disable-RoseeyesNvidiaPipIndex

$resolvedSetup = Get-RoseeyesRosSetup -Preferred $RosSetup
if (-not $resolvedSetup) {
  throw "ROS 2 setup.ps1 not found. Set ROS2_WINDOWS_SETUP or pass -RosSetup."
}
. (Join-Path $PSScriptRoot "Activate-Ros.ps1") -RosSetup $resolvedSetup

$RosPython = Get-RoseeyesRosPython
if (-not (Test-Path $Requirements)) {
  throw "Missing $Requirements"
}

Write-Host "ROS Python: $RosPython"
Write-Host "Upgrading pip..."
$pipCode = Invoke-RoseeyesPip -Python $RosPython -PipArgs @(
  "install", "--disable-pip-version-check", "-U", "pip"
)
if ($pipCode -ne 0) { throw "pip self-upgrade failed ($pipCode)" }

Write-Host "Installing/updating $Requirements ..."
$reqCode = Invoke-RoseeyesPip -Python $RosPython -PipArgs @(
  "install", "--disable-pip-version-check", "-U", "-r", $Requirements
)
if ($reqCode -ne 0) { throw "pip install -r requirements failed ($reqCode)" }

$stamp = Join-Path $PythonDir ".roseyes_ros_pygame.sha256"
$wantHash = (Get-FileHash -Algorithm SHA256 -Path $Requirements).Hash.ToLowerInvariant()
Set-Content -Path $stamp -Value $wantHash -NoNewline

Write-Host "Verifying imports..."
& $RosPython -c "import pygame; import PIL; print('pygame', pygame.__version__, 'Pillow', PIL.__version__)"
if ($LASTEXITCODE -ne 0) { throw "Import check failed ($LASTEXITCODE)" }

if (-not $SkipPlatformIoPip) {
  Update-RoseeyesPlatformIoPip
}

Write-Host "Python env ready. ROS_DISTRO=$env:ROS_DISTRO  python=$RosPython"

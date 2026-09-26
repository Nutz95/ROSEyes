<#
.SYNOPSIS
  Xbox left stick -> /eyes/gaze via Windows ROS 2 + native MicroXRCEAgent.
.DESCRIPTION
  Activates host ROS (ROS2_WINDOWS_SETUP), ensures pygame on that Python,
  starts the Windows agent if needed, runs xbox_gaze_publisher.py. No Docker.
.EXAMPLE
  .\scripts\Start-XboxGaze.ps1
#>
param(
  [double]$RateHz = 15,
  [double]$Deadzone = 0.12,
  [switch]$SkipAgentStart,
  [switch]$NoInvertY,
  [switch]$ForcePipInstall,
  [string]$RosSetup = $env:ROS2_WINDOWS_SETUP
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$PythonDir = Join-Path $RepoRoot "python"
$Publisher = Join-Path $PythonDir "xbox_gaze_publisher.py"
$Requirements = Join-Path $PythonDir "requirements.txt"
. (Join-Path $PSScriptRoot "Common.ps1")

if (-not (Test-Path $Publisher)) {
  throw "Missing $Publisher"
}

$resolvedSetup = Get-RoseeyesRosSetup -Preferred $RosSetup
if (-not $resolvedSetup) {
  throw "ROS 2 setup.ps1 not found. Set ROS2_WINDOWS_SETUP or pass -RosSetup."
}
. (Join-Path $PSScriptRoot "Activate-Ros.ps1") -RosSetup $resolvedSetup

$RosPython = Get-RoseeyesRosPython

$stamp = Join-Path $PythonDir ".roseyes_ros_pygame.sha256"
$wantHash = (Get-FileHash -Algorithm SHA256 -Path $Requirements).Hash.ToLowerInvariant()
$haveHash = ""
if (Test-Path $stamp) { $haveHash = (Get-Content -Raw $stamp).Trim() }
$needPygame = $ForcePipInstall -or ($haveHash -ne $wantHash)
if (-not $needPygame) {
  & $RosPython -c "import pygame" 2>$null
  if ($LASTEXITCODE -ne 0) { $needPygame = $true }
}
if ($needPygame) {
  Write-Host "Installing host Python deps from requirements.txt..."
  $code = Invoke-RoseeyesPip -Python $RosPython -PipArgs @(
    "install", "-U", "-r", $Requirements
  )
  if ($code -ne 0) {
    throw "pip install -r requirements failed ($code)"
  }
  Set-Content -Path $stamp -Value $wantHash -NoNewline
}

if (-not $SkipAgentStart) {
  & (Join-Path $PSScriptRoot "Start-MicroRosAgent.ps1")
  if ($LASTEXITCODE -ne 0) {
    Write-Warning "Agent start returned $LASTEXITCODE - continuing anyway."
  }
}

$pubArgs = @($Publisher, "--rate-hz", "$RateHz", "--deadzone", "$Deadzone")
if ($NoInvertY) { $pubArgs += "--no-invert-y" }

Write-Host "Xbox -> host ROS ($env:ROS_DISTRO) -> MicroXRCEAgent -> ESP"
Write-Host "Move left stick; A = blink. Ctrl+C to stop."
Push-Location $PythonDir
try {
  & $RosPython @pubArgs
  $global:LASTEXITCODE = [int]$LASTEXITCODE
} finally {
  Pop-Location
}

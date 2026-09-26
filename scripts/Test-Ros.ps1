<#
.SYNOPSIS
  Ensures the micro-ROS agent is up and publishes sample gaze via Windows ROS.
.DESCRIPTION
  Uses host ROS 2 + native MicroXRCEAgent. Requires ROS2_WINDOWS_SETUP.
.PARAMETER RosSetup
  Optional path to ROS setup.ps1. Otherwise uses $env:ROS2_WINDOWS_SETUP.
.EXAMPLE
  .\scripts\Test-Ros.ps1
.EXAMPLE
  .\scripts\Test-Ros.ps1 -FixedGaze -GazeX 0.8 -GazeY -0.4 -Count 20
#>
param(
  [double]$GazeX = 0.6,
  [double]$GazeY = -0.2,
  [int]$Count = 40,
  [switch]$SkipAgentStart,
  [switch]$FixedGaze,
  [string]$RosSetup = $env:ROS2_WINDOWS_SETUP
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$originalLocation = Get-Location
. (Join-Path $PSScriptRoot "Common.ps1")

try {
  if (-not $SkipAgentStart) {
    & (Join-Path $PSScriptRoot "Start-MicroRosAgent.ps1")
    if ($LASTEXITCODE -ne 0) {
      Write-Warning "Agent start returned $LASTEXITCODE - continuing anyway."
    }
  }

  $resolvedSetup = Get-RoseeyesRosSetup -Preferred $RosSetup
  if (-not $resolvedSetup) {
    throw "ROS 2 setup.ps1 not found. Set ROS2_WINDOWS_SETUP or pass -RosSetup."
  }
  . (Join-Path $PSScriptRoot "Activate-Ros.ps1") -RosSetup $resolvedSetup

  $useSweep = -not $FixedGaze
  if ($useSweep) {
    Write-Host "Publishing $Count samples as a left/right/center sweep (host ROS)"
  } else {
    Write-Host "Publishing $Count samples to /eyes/gaze (x=$GazeX y=$GazeY) via host ROS"
  }

  $PublishScript = Join-Path $RepoRoot "python\publish_test_gaze.py"
  $RosPython = Get-RoseeyesRosPython
  Write-Host "Using Python: $RosPython"
  Push-Location (Join-Path $RepoRoot "python")
  try {
    $pyArgs = @($PublishScript, "--count", $Count, "--period-s", "0.08")
    if ($useSweep) { $pyArgs += "--sweep" }
    else { $pyArgs += @("--gaze-x", "$GazeX", "--gaze-y", "$GazeY") }
    & $RosPython @pyArgs
    $global:LASTEXITCODE = [int]$LASTEXITCODE
  } finally {
    Pop-Location
  }
} finally {
  Set-Location $originalLocation
}

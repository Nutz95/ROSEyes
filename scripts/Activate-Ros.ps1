<#
.SYNOPSIS
  Dot-sources the ROS 2 environment on this machine.
.DESCRIPTION
  Requires -RosSetup or environment variable ROS2_WINDOWS_SETUP pointing at
  setup.ps1. Temporarily uses Process-scoped Bypass so unsigned vendor setup
  scripts can load under AllSigned/Restricted policies.
.PARAMETER RosSetup
  Path to ROS 2 setup.ps1. Defaults to $env:ROS2_WINDOWS_SETUP.
.EXAMPLE
  . .\scripts\Activate-Ros.ps1
.EXAMPLE
  . .\scripts\Activate-Ros.ps1 -RosSetup "D:\ros2\setup.ps1"
#>
param(
  [string]$RosSetup = $env:ROS2_WINDOWS_SETUP
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "Common.ps1")

$resolved = Get-RoseeyesRosSetup -Preferred $RosSetup
if (-not $resolved) {
  throw @"
ROS 2 setup path not set or missing.
Set user/machine env ROS2_WINDOWS_SETUP to your setup.ps1, or pass -RosSetup.
Current ROS2_WINDOWS_SETUP='$env:ROS2_WINDOWS_SETUP'
"@
}

Write-Host "Activating ROS 2 from $resolved"
Invoke-RoseeyesDotSource -Path $resolved

$rosRoot = Split-Path -Parent $resolved
$pixiEnv = Join-Path $rosRoot ".pixi\envs\default"
if (Test-Path $pixiEnv) {
  $pixiBins = @(
    $pixiEnv
    (Join-Path $pixiEnv "Library\bin")
    (Join-Path $pixiEnv "Scripts")
  ) -join ";"
  $env:PATH = "$pixiBins;$env:PATH"
  $env:ROSEYES_ROS_PYTHON = Join-Path $pixiEnv "python.exe"
  Write-Host "Prepended pixi bins for rclpy native libs"
}

if (-not $env:ROS_DISTRO) {
  Write-Warning "ROS_DISTRO is still empty after setup. Check the install."
} else {
  Write-Host "ROS_DISTRO=$env:ROS_DISTRO"
}

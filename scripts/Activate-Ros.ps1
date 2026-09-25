<#
.SYNOPSIS
  Dot-sources the ROS 2 lyrical environment on this machine.
.DESCRIPTION
  Default install path: I:\ROS\ros2-windows\setup.ps1
  Override with ROS2_WINDOWS_SETUP.
#>
param(
  [string]$RosSetup = $env:ROS2_WINDOWS_SETUP
)

$ErrorActionPreference = "Stop"

if (-not $RosSetup -or $RosSetup.Trim() -eq "") {
  $RosSetup = "I:\ROS\ros2-windows\setup.ps1"
}

if (-not (Test-Path $RosSetup)) {
  throw "ROS 2 setup script not found: $RosSetup"
}

Write-Host "Activating ROS 2 from $RosSetup"
. $RosSetup

if (-not $env:ROS_DISTRO) {
  Write-Warning "ROS_DISTRO is still empty after setup. Check the lyrical install."
} else {
  Write-Host "ROS_DISTRO=$env:ROS_DISTRO"
}

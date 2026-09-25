<#
.SYNOPSIS
  Activates ROS 2, ensures the micro-ROS agent is up, and publishes sample gaze commands.
#>
param(
  [double]$GazeX = 0.6,
  [double]$GazeY = -0.2,
  [int]$Count = 20,
  [switch]$SkipAgentStart
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot

. (Join-Path $PSScriptRoot "Activate-Ros.ps1")

if (-not $SkipAgentStart) {
  & (Join-Path $PSScriptRoot "Start-MicroRosAgent.ps1")
  if ($LASTEXITCODE -ne 0) {
    Write-Warning "Agent start returned $LASTEXITCODE - continuing anyway."
  }
}

$PublishScript = Join-Path $RepoRoot "python\publish_test_gaze.py"
Write-Host "Publishing $Count samples to /eyes/gaze (x=$GazeX y=$GazeY)"
Set-Location (Join-Path $RepoRoot "python")
python $PublishScript --gaze-x $GazeX --gaze-y $GazeY --count $Count
$exitCode = $LASTEXITCODE

if ($exitCode -ne 0) {
  Write-Host ""
  Write-Host "Host rclpy publish failed (exit $exitCode). Fallback via Docker:"
  Write-Host "  docker run --rm --network container:roseyes-micro-ros-agent ros:jazzy-ros-base \"
  Write-Host "    bash -lc `"source /opt/ros/jazzy/setup.bash && ros2 topic pub --once /eyes/gaze geometry_msgs/msg/Vector3 '{x: 0.5, y: 0.0, z: 0.0}'`""
}

exit $exitCode

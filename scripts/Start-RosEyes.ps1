<#
.SYNOPSIS
  Interactive menu: debug cockpit, Xbox, mode, echoes, agent.
.DESCRIPTION
  Central host-ROS entry for ROSEyes. Activates ROS2_WINDOWS_SETUP once,
  starts MicroXRCEAgent unless -SkipAgentStart, then loops a menu until Quit.
  Press q inside Xbox/echo tools (or Ctrl+C) to return to this menu.
.EXAMPLE
  .\scripts\Start-RosEyes.ps1
.EXAMPLE
  .\scripts\Start-RosEyes.ps1 -SkipAgentStart
#>
param(
  [switch]$SkipAgentStart,
  [string]$RosSetup = $env:ROS2_WINDOWS_SETUP
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$PythonDir = Join-Path $RepoRoot "python"
. (Join-Path $PSScriptRoot "Common.ps1")

function Write-Menu {
  Write-Host ""
  Write-Host "======== ROSEyes host ROS ========" -ForegroundColor Cyan
  Write-Host " 1  Debug cockpit GUI   (gaze pad/Xbox, ball, JPEG, telemetry)"
  Write-Host " 2  Xbox pilot only     (/eyes/mode piloted + stick -> gaze)"
  Write-Host " 3  Test gaze sweep     (publish sample /eyes/gaze)"
  Write-Host " 4  Mode autonomous"
  Write-Host " 5  Mode piloted"
  Write-Host " 6  Echo /eyes/status"
  Write-Host " 7  Echo /eyes/ball"
  Write-Host " 8  Echo /eyes/gaze"
  Write-Host " 9  Echo /eyes/perf"
  Write-Host "10  Blink once"
  Write-Host "11  List eyes topics"
  Write-Host "12  Restart MicroXRCEAgent"
  Write-Host " 0  Quit"
  Write-Host "=================================="
  Write-Host "Tip: close the GUI window to return here. In CLI tools press q / Ctrl+C."
}

function Ensure-RosActivated {
  param([string]$PreferredSetup)
  $resolved = Get-RoseeyesRosSetup -Preferred $PreferredSetup
  if (-not $resolved) {
    throw "ROS 2 setup.ps1 not found. Set ROS2_WINDOWS_SETUP or pass -RosSetup."
  }
  . (Join-Path $PSScriptRoot "Activate-Ros.ps1") -RosSetup $resolved
  Write-Host "ROS distro=$env:ROS_DISTRO  python=$(Get-RoseeyesRosPython)"
}

function Invoke-AgentStart {
  & (Join-Path $PSScriptRoot "Start-MicroRosAgent.ps1")
  if ($LASTEXITCODE -ne 0) {
    Write-Warning "Agent start returned $LASTEXITCODE"
  }
}

function Publish-EyesMode {
  param([ValidateSet("autonomous", "piloted")][string]$Mode)
  Write-Host "Publishing /eyes/mode data=$Mode"
  ros2 topic pub --once /eyes/mode std_msgs/msg/String "{data: $Mode}"
}

function Invoke-TopicEcho {
  param([ValidateSet("status", "ball", "gaze", "perf")][string]$TopicShort)
  $RosPython = Get-RoseeyesRosPython
  $echoScript = Join-Path $PythonDir "echo_eyes_topic.py"
  Write-Host "Echo /eyes/$TopicShort - press q to return to menu"
  Push-Location $PythonDir
  try {
    & $RosPython $echoScript $TopicShort
  } finally {
    Pop-Location
  }
}

function Invoke-DebugCockpit {
  $RosPython = Get-RoseeyesRosPython
  $ui = Join-Path $PythonDir "roseyes_debug_ui.py"
  Write-Host "Debug cockpit - close the window to return to menu"
  Write-Host "If imports fail: .\scripts\0_setup_python_env.ps1"
  Push-Location $PythonDir
  try {
    & $RosPython $ui
  } finally {
    Pop-Location
  }
}

function Invoke-MenuAction {
  param([scriptblock]$Action)
  try {
    & $Action
  } catch {
    if ($_.Exception -is [System.Management.Automation.PipelineStoppedException] -or
        $_.FullyQualifiedErrorId -eq "NativeCommandAborted" -or
        $_.CategoryInfo.Category -eq "OperationStopped") {
      Write-Host "Back to menu."
      return
    }
    Write-Warning $_.Exception.Message
  }
}

Ensure-RosActivated -PreferredSetup $RosSetup

# Keep the menu alive when the user hits Ctrl+C during a child tool.
$cancelHandler = [System.ConsoleCancelEventHandler] {
  param($sender, $eventArgs)
  $eventArgs.Cancel = $true
  Write-Host ""
  Write-Host "Ctrl+C - returning to menu (press 0 to quit)." -ForegroundColor Yellow
}
[Console]::add_CancelKeyPress($cancelHandler)

try {
  if (-not $SkipAgentStart) {
    Invoke-AgentStart
  } else {
    Write-Host "Skipping agent start (-SkipAgentStart)"
  }

  $quit = $false
  while (-not $quit) {
    Write-Menu
    $choice = Read-Host "Choice"
    switch ($choice) {
      "1" { Invoke-MenuAction { Invoke-DebugCockpit } }
      "2" {
        Invoke-MenuAction {
          Publish-EyesMode -Mode piloted
          & (Join-Path $PSScriptRoot "Start-XboxGaze.ps1") -SkipAgentStart
        }
      }
      "3" {
        Invoke-MenuAction {
          & (Join-Path $PSScriptRoot "Test-Ros.ps1") -SkipAgentStart
        }
      }
      "4" { Publish-EyesMode -Mode autonomous }
      "5" { Publish-EyesMode -Mode piloted }
      "6" { Invoke-MenuAction { Invoke-TopicEcho -TopicShort status } }
      "7" { Invoke-MenuAction { Invoke-TopicEcho -TopicShort ball } }
      "8" { Invoke-MenuAction { Invoke-TopicEcho -TopicShort gaze } }
      "9" { Invoke-MenuAction { Invoke-TopicEcho -TopicShort perf } }
      "10" {
        ros2 topic pub --once /eyes/blink std_msgs/msg/Empty "{}"
      }
      "11" {
        ros2 topic list | Where-Object { $_ -match "eyes" }
      }
      "12" { Invoke-AgentStart }
      "0" { $quit = $true }
      default { Write-Warning "Unknown choice: $choice" }
    }
  }
} finally {
  [Console]::remove_CancelKeyPress($cancelHandler)
}

Write-Host "Bye."

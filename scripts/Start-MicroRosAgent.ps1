<#
.SYNOPSIS
  Starts the native Windows micro-ROS / MicroXRCEAgent on UDP 8888.
.DESCRIPTION
  Resolves MicroXRCEAgent.exe from MICROROS_AGENT_EXE, MICROROS_AGENT_HOME,
  a sibling of ROS2_WINDOWS_SETUP, %LOCALAPPDATA%\ROSEyes\MicroXRCEAgent, or PATH.
  Stops the legacy Docker agent container if it is still running.
  First-time setup: .\scripts\Install-MicroRosAgent.ps1
.EXAMPLE
  .\scripts\Start-MicroRosAgent.ps1
#>
param(
  [int]$Port = 8888,
  [string]$AgentExe = $env:MICROROS_AGENT_EXE,
  [string]$ContainerName = "roseyes-micro-ros-agent",
  [switch]$KeepDockerAgent
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "Common.ps1")

function Stop-LegacyDockerAgent {
  param([string]$Name)
  if (-not (Get-Command docker -ErrorAction SilentlyContinue)) { return }
  $running = docker ps --filter "name=^/${Name}$" --format "{{.Names}}" 2>$null
  if ($running -eq $Name) {
    Write-Host "Stopping legacy Docker agent '$Name' (Windows agent replaces it)..."
    docker stop $Name | Out-Null
  }
}

$exe = Resolve-RoseeyesMicroXrceAgentExe -Preferred $AgentExe
if (-not $exe) {
  $agentHome = Get-RoseeyesMicroXrceAgentHome
  throw @"
MicroXRCEAgent.exe not found.
Build it once (Visual Studio + CMake):
  .\scripts\Install-MicroRosAgent.ps1
Or set MICROROS_AGENT_EXE / MICROROS_AGENT_HOME.
Expected under: $agentHome
"@
}

if (-not $KeepDockerAgent) {
  Stop-LegacyDockerAgent -Name $ContainerName
}

Get-CimInstance Win32_Process -Filter "Name = 'MicroXRCEAgent.exe'" -ErrorAction SilentlyContinue |
  ForEach-Object {
    Write-Host "Stopping existing MicroXRCEAgent PID $($_.ProcessId)..."
    Stop-Process -Id $_.ProcessId -Force -ErrorAction SilentlyContinue
  }

$hostIp = Get-PreferredLanIPv4
Write-Host "Starting $exe"
Write-Host "  udp4 --port $Port -v4"
if ($hostIp) {
  Write-Host "Flash / seed MICROROS_AGENT_IP=$hostIp (ESP must reach this host:$Port/udp)"
} else {
  Write-Warning "Could not detect LAN IPv4 - set MICROROS_AGENT_IP manually before flashing."
}
Write-Host "OTA is board-direct. After WiFi is up, use -OtaIp with the board IP."

$agentDir = Split-Path -Parent $exe
foreach ($extra in @($agentDir, (Join-Path $agentDir ".."), (Split-Path -Parent $agentDir))) {
  $resolved = [System.IO.Path]::GetFullPath($extra)
  if (Test-Path $resolved) {
    $env:PATH = "$resolved;$env:PATH"
  }
}

Start-Process -FilePath $exe -ArgumentList @("udp4", "--port", "$Port", "-v4") `
  -WorkingDirectory $agentDir `
  -WindowStyle Minimized
Start-Sleep -Milliseconds 500
$alive = Get-Process -Name "MicroXRCEAgent" -ErrorAction SilentlyContinue
if (-not $alive) {
  throw "MicroXRCEAgent failed to stay running. Try launching manually: `"$exe`" udp4 --port $Port -v4"
}
Write-Host "MicroXRCEAgent running (PID $($alive[0].Id))."
exit 0

<#
.SYNOPSIS
  Starts the micro-ROS agent (Docker, jazzy, UDPv4 port 8888).

.NOTES
  The ESP must use the Windows host LAN IPv4 as MICROROS_AGENT_IP (auto-detected
  by Build-And-Upload* when unset). Docker only publishes UDP 8888 on the host;
  ArduinoOTA is separate (TCP/UDP 3232 straight to the board).
#>
param(
  [int]$Port = 8888,
  [string]$ContainerName = "roseyes-micro-ros-agent"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$ComposeFile = Join-Path $RepoRoot "docker\docker-compose.agent.yml"
. (Join-Path $PSScriptRoot "Common.ps1")

$existing = docker ps -a --filter "name=^/${ContainerName}$" --format "{{.Names}}" 2>$null
if ($existing -eq $ContainerName) {
  $running = docker ps --filter "name=^/${ContainerName}$" --format "{{.Names}}" 2>$null
  if ($running -eq $ContainerName) {
    Write-Host "Agent container '$ContainerName' already running."
  } else {
    Write-Host "Starting existing container '$ContainerName'..."
    docker start $ContainerName
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  }
} else {
  Write-Host "Launching micro-ROS agent on UDP $Port via docker compose..."
  docker compose -f $ComposeFile up -d
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

$hostIp = Get-PreferredLanIPv4
if ($hostIp) {
  Write-Host "Flash / seed MICROROS_AGENT_IP=$hostIp (ESP must reach this host:8888/udp)"
} else {
  Write-Warning "Could not detect LAN IPv4 — set MICROROS_AGENT_IP manually before flashing."
}
Write-Host "OTA is board-direct (not Docker). After WiFi up, use -OtaIp <board-ip>."
exit 0

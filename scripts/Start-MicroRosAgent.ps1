<#
.SYNOPSIS
  Starts the micro-ROS agent (Docker, jazzy, UDPv4 port 8888).
#>
param(
  [int]$Port = 8888,
  [string]$ContainerName = "roseyes-micro-ros-agent"
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$ComposeFile = Join-Path $RepoRoot "docker\docker-compose.agent.yml"

$existing = docker ps -a --filter "name=^/${ContainerName}$" --format "{{.Names}}" 2>$null
if ($existing -eq $ContainerName) {
  $running = docker ps --filter "name=^/${ContainerName}$" --format "{{.Names}}" 2>$null
  if ($running -eq $ContainerName) {
    Write-Host "Agent container '$ContainerName' already running."
    exit 0
  }
  Write-Host "Starting existing container '$ContainerName'..."
  docker start $ContainerName
  exit $LASTEXITCODE
}

Write-Host "Launching micro-ROS agent on UDP $Port via docker compose..."
docker compose -f $ComposeFile up -d
exit $LASTEXITCODE

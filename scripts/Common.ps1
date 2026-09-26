# Escape a string for safe inclusion inside a C compiler -D"NAME=..." flag.
function Escape-CString {
  param([string]$Value)
  if ($null -eq $Value) { return "" }
  $escaped = $Value.Replace('\', '\\').Replace('"', '\"')
  return $escaped
}

# Resolves ROS 2 setup.ps1 from -Preferred or $env:ROS2_WINDOWS_SETUP only.
function Get-RoseeyesRosSetup {
  param([string]$Preferred = "")
  foreach ($c in @($Preferred, $env:ROS2_WINDOWS_SETUP)) {
    if ($c -and $c.Trim() -ne "" -and (Test-Path $c)) {
      return (Resolve-Path $c).Path
    }
  }
  return $null
}

# Python that can import rclpy after Activate-Ros.ps1 (override via ROSEYES_ROS_PYTHON).
function Get-RoseeyesRosPython {
  if ($env:ROSEYES_ROS_PYTHON -and (Test-Path $env:ROSEYES_ROS_PYTHON)) {
    return $env:ROSEYES_ROS_PYTHON
  }
  return "python"
}

# Default install root for native MicroXRCEAgent (no hardcoded drive letters).
# Order: MICROROS_AGENT_HOME, sibling of ROS2_WINDOWS_SETUP's parent, LOCALAPPDATA.
function Get-RoseeyesMicroXrceAgentHome {
  if ($env:MICROROS_AGENT_HOME -and $env:MICROROS_AGENT_HOME.Trim() -ne "") {
    return $env:MICROROS_AGENT_HOME.Trim()
  }
  $rosSetup = Get-RoseeyesRosSetup
  if ($rosSetup) {
    # .../ros2-windows/setup.ps1 -> .../MicroXRCEAgent
    $rosInstall = Split-Path -Parent $rosSetup
    $rosParent = Split-Path -Parent $rosInstall
    if ($rosParent) {
      return (Join-Path $rosParent "MicroXRCEAgent")
    }
  }
  return (Join-Path $env:LOCALAPPDATA "ROSEyes\MicroXRCEAgent")
}

# Locates MicroXRCEAgent.exe via MICROROS_AGENT_EXE, agent home, repo tools, PATH.
function Resolve-RoseeyesMicroXrceAgentExe {
  param([string]$Preferred = "")
  $candidates = @()
  if ($Preferred) { $candidates += $Preferred }
  if ($env:MICROROS_AGENT_EXE) { $candidates += $env:MICROROS_AGENT_EXE }

  $agentHome = Get-RoseeyesMicroXrceAgentHome
  $candidates += @(
    (Join-Path $agentHome "bin\MicroXRCEAgent.exe")
    (Join-Path $agentHome "MicroXRCEAgent.exe")
  )

  $repoRoot = Split-Path -Parent $PSScriptRoot
  $tools = Join-Path $repoRoot "tools\MicroXRCEAgent"
  if (Test-Path $tools) {
    $hit = Get-ChildItem $tools -Recurse -Filter "MicroXRCEAgent.exe" -ErrorAction SilentlyContinue |
      Select-Object -First 1
    if ($hit) { $candidates += $hit.FullName }
  }

  $fromPath = Get-Command MicroXRCEAgent.exe -ErrorAction SilentlyContinue
  if ($fromPath) { $candidates += $fromPath.Source }

  foreach ($c in $candidates) {
    if ($c -and (Test-Path $c)) { return (Resolve-Path $c).Path }
  }
  return $null
}

# Dot-sources an unsigned .ps1 under Process Bypass (ROS setup.ps1 is rarely signed).
function Invoke-RoseeyesDotSource {
  param([Parameter(Mandatory = $true)][string]$Path)
  if (-not (Test-Path $Path)) {
    throw "Script not found: $Path"
  }
  $previous = Get-ExecutionPolicy -Scope Process
  try {
    Set-ExecutionPolicy -Scope Process -ExecutionPolicy Bypass -Force
    . $Path
  } finally {
    try {
      Set-ExecutionPolicy -Scope Process -ExecutionPolicy $previous -Force
    } catch {
      # Ignore restore failures on locked policies.
    }
  }
}

function Test-IsLikelyWslOrHyperVAddress {
  param([string]$IpAddress)
  # WSL2 / Hyper-V host NICs commonly land in 172.16.0.0/12
  if ($IpAddress -match '^172\.(1[6-9]|2[0-9]|3[0-1])\.') { return $true }
  if ($IpAddress -like '169.254.*') { return $true }
  return $false
}

# Resolve the LAN IPv4 address of this machine (prefer non-loopback Ethernet/Wi-Fi).
function Get-PreferredLanIPv4 {
  $candidates = Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue |
    Where-Object {
      $_.IPAddress -notlike '127.*' -and
      $_.PrefixOrigin -ne 'WellKnown' -and
      $_.AddressState -eq 'Preferred' -and
      -not (Test-IsLikelyWslOrHyperVAddress $_.IPAddress)
    } |
    Sort-Object -Property InterfaceMetric

  if ($candidates) {
    return $candidates[0].IPAddress
  }

  # Fallback: any non-loopback preferred address
  $fallback = Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue |
    Where-Object {
      $_.IPAddress -notlike '127.*' -and
      $_.AddressState -eq 'Preferred'
    } |
    Sort-Object -Property InterfaceMetric
  if ($fallback) {
    return $fallback[0].IPAddress
  }
  return $null
}

# Lists likely ESP32 USB serial ports (Espressif VID 303A).
function Get-EspUsbSerialPorts {
  Get-PnpDevice -Class Ports -Status OK -ErrorAction SilentlyContinue |
    Where-Object { $_.InstanceId -match 'VID_303A' } |
    ForEach-Object {
      if ($_.FriendlyName -match '(COM\d+)') { $Matches[1] }
    }
}

# Writes firmware/include/NetworkSeedSecrets.generated.h from env vars.
function Set-RoseeyesSeedBuildFlags {
  param(
    [switch]$RequireAgentIp,
    [string]$FirmwareIncludeDir = ""
  )

  $WifiSsid = $env:WIFI_SSID
  $WifiPass = $env:WIFI_PASS
  $AgentIp = $env:MICROROS_AGENT_IP

  if (-not $FirmwareIncludeDir) {
    $FirmwareIncludeDir = Join-Path (Split-Path -Parent $PSScriptRoot) "firmware\include"
    if (-not (Test-Path $FirmwareIncludeDir)) {
      $FirmwareIncludeDir = Join-Path $PSScriptRoot "..\firmware\include"
    }
  }

  Remove-Item Env:PLATFORMIO_BUILD_FLAGS -ErrorAction SilentlyContinue

  if (-not $WifiSsid -or -not $WifiPass) {
    if ($RequireAgentIp) {
      throw "WIFI_SSID and WIFI_PASS environment variables are required."
    }
    return $false
  }

  if (-not $AgentIp) {
    $AgentIp = Get-PreferredLanIPv4
  }
  if ($RequireAgentIp -and -not $AgentIp) {
    throw "MICROROS_AGENT_IP is not set and no LAN IPv4 could be detected."
  }
  if (-not $AgentIp) {
    $AgentIp = "0.0.0.0"
  }

  $AgentPort = 8888
  if ($env:MICROROS_AGENT_PORT) {
    $AgentPort = [int]$env:MICROROS_AGENT_PORT
  }

  $SsidEsc = Escape-CString $WifiSsid
  $PassEsc = Escape-CString $WifiPass
  $IpEsc = Escape-CString $AgentIp

  $generated = Join-Path $FirmwareIncludeDir "NetworkSeedSecrets.generated.h"
  $content = @(
    '#pragma once',
    "// Generated by Build-And-Upload scripts. Do not commit.",
    "#define WIFI_SSID_SEED `"$SsidEsc`"",
    "#define WIFI_PASS_SEED `"$PassEsc`"",
    "#define MICROROS_AGENT_IP_SEED `"$IpEsc`"",
    "#define MICROROS_AGENT_PORT_SEED $AgentPort"
  ) -join "`n"

  New-Item -ItemType Directory -Force -Path $FirmwareIncludeDir | Out-Null
  [System.IO.File]::WriteAllText($generated, $content + "`n")
  Write-Host "Wrote NVS seed header: $generated (SSID length=$($WifiSsid.Length), agent=$AgentIp`:$AgentPort)"
  return $true
}

# PlatformIO venv Python (Windows).
function Get-RoseeyesPlatformIoPython {
  $python = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\python.exe"
  if (Test-Path $python) { return $python }
  return $null
}

# Avoid cp1252 UnicodeEncodeError when PIO/esptool print progress; silence pip nag.
function Set-RoseeyesPythonIoUtf8 {
  $env:PYTHONUTF8 = "1"
  $env:PYTHONIOENCODING = "utf-8"
  $env:PIP_DISABLE_PIP_VERSION_CHECK = "1"
  try {
    [Console]::OutputEncoding = [System.Text.UTF8Encoding]::new($false)
    $global:OutputEncoding = [System.Text.UTF8Encoding]::new($false)
  } catch {
    # Non-interactive hosts may lack a console.
  }
}

# Pip + flash helpers (kept loadable via Common.ps1 for existing scripts).
. (Join-Path $PSScriptRoot "RoseeyesPip.ps1")
. (Join-Path $PSScriptRoot "RoseeyesFlash.ps1")

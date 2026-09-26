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

# Maps base env name to manual/ota variant used by this PlatformIO version.
function Get-RoseeyesUploadEnvName {
  param(
    [Parameter(Mandatory = $true)][string]$EnvName,
    [string]$OtaIp = "",
    [switch]$ManualBootloader
  )
  if ($OtaIp) { return "${EnvName}_ota" }
  if ($ManualBootloader) { return "${EnvName}_manual" }
  return $EnvName
}

# Locates Arduino espota.py under the PlatformIO packages tree.
function Get-RoseeyesEspotaScript {
  $packages = Join-Path $env:USERPROFILE ".platformio\packages"
  $hit = Get-ChildItem -Path $packages -Recurse -Filter "espota.py" -ErrorAction SilentlyContinue |
    Select-Object -First 1
  if (-not $hit) {
    throw "espota.py not found under $packages (install an espressif32 PlatformIO platform once)."
  }
  return $hit.FullName
}

# OTA-upload a prebuilt firmware.bin without invoking PlatformIO rebuild
# (critical after WSL micro-ROS builds — Windows cannot rebuild libmicroros).
function Invoke-RoseeyesPrebuiltOtaUpload {
  param(
    [Parameter(Mandatory = $true)][string]$FirmwareBin,
    [Parameter(Mandatory = $true)][string]$OtaIp,
    [int]$OtaPort = 3232
  )

  if (-not (Test-Path $FirmwareBin)) {
    throw "Prebuilt firmware not found: $FirmwareBin"
  }

  $espota = Get-RoseeyesEspotaScript
  $python = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\python.exe"
  if (-not (Test-Path $python)) {
    $python = "python"
  }

  Write-Host "OTA upload (prebuilt, no rebuild) via espota to ${OtaIp}:$OtaPort"
  Write-Host "  bin: $FirmwareBin"
  Write-Host "  espota: $espota"
  # espota prints progress on stderr; with $ErrorActionPreference=Stop that becomes a
  # terminating ErrorRecord when merged via 2>&1 — temporarily continue.
  $prevEap = $ErrorActionPreference
  $ErrorActionPreference = "Continue"
  try {
    $otaOut = & $python $espota -i $OtaIp -p $OtaPort -f $FirmwareBin 2>&1
    $otaCode = [int]$LASTEXITCODE
  } finally {
    $ErrorActionPreference = $prevEap
  }
  foreach ($line in $otaOut) {
    Write-Host ([string]$line)
  }
  if ($otaCode -ne 0) {
    Write-Error "espota failed (exit $otaCode)"
  } else {
    Write-Host "OTA finished OK (exit 0)"
  }
  return $otaCode
}

# Uploads via serial or espota (dedicated PlatformIO envs, no --project-option).
# Always uses -d ProjectDir so the caller's working directory is unchanged.
# For WSL-built micro-ROS images, prefer Invoke-RoseeyesPrebuiltOtaUpload instead.
function Invoke-RoseeyesUpload {
  param(
    [Parameter(Mandatory = $true)][string]$Pio,
    [Parameter(Mandatory = $true)][string]$ProjectDir,
    [Parameter(Mandatory = $true)][string]$EnvName,
    [string]$Port = "COM16",
    [string]$OtaIp = "",
    [switch]$ManualBootloader
  )

  $uploadEnv = Get-RoseeyesUploadEnvName -EnvName $EnvName -OtaIp $OtaIp -ManualBootloader:$ManualBootloader

  if ($OtaIp) {
    Write-Host "OTA upload via env=$uploadEnv to $OtaIp ..."
    & $Pio run -d $ProjectDir -e $uploadEnv -t upload --upload-port $OtaIp
    return [int]$LASTEXITCODE
  }

  $espPorts = @(Get-EspUsbSerialPorts)
  if ($espPorts.Count -gt 0 -and ($espPorts -notcontains $Port)) {
    Write-Warning "Port $Port is not an Espressif USB device. Detected: $($espPorts -join ', ')"
  }

  Write-Host "Serial upload via env=$uploadEnv to $Port ..."
  if ($ManualBootloader) {
    Write-Host "Manual bootloader: hold B, tap R, release B, then press Enter."
    [void](Read-Host)
  } else {
    Write-Host "Auto-reset enabled (Arduino IDE style: 1200bps touch + wait for port)."
    Write-Host "Close Serial Monitor first. Fallback only if needed: -ManualBootloader"
  }

  & $Pio run -d $ProjectDir -e $uploadEnv -t upload --upload-port $Port
  $uploadExit = [int]$LASTEXITCODE
  if ($uploadExit -ne 0 -and -not $ManualBootloader) {
    Write-Host "First upload attempt exit=$uploadExit - retrying once after 2s..."
    Start-Sleep -Seconds 2
    $espPorts = @(Get-EspUsbSerialPorts)
    if ($espPorts.Count -eq 1) {
      $Port = $espPorts[0]
      Write-Host "Using re-detected port $Port"
    }
    & $Pio run -d $ProjectDir -e $uploadEnv -t upload --upload-port $Port
    $uploadExit = [int]$LASTEXITCODE
  }

  Write-Host "Upload finished with exit code $uploadExit"
  return $uploadExit
}

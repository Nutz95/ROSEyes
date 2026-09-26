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

# Locates esptool.py under the PlatformIO packages tree.
function Get-RoseeyesEsptoolScript {
  $packages = Join-Path $env:USERPROFILE ".platformio\packages"
  $hit = Get-ChildItem -Path $packages -Recurse -Filter "esptool.py" -ErrorAction SilentlyContinue |
    Select-Object -First 1
  if (-not $hit) {
    throw "esptool.py not found under $packages (install an espressif32 PlatformIO platform once)."
  }
  return $hit.FullName
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

# Paths where NVIDIA PyIndex may have injected pypi.ngc.nvidia.com.
function Get-RoseeyesUserPipIniPaths {
  @(
    (Join-Path $env:APPDATA "pip\pip.ini")
    (Join-Path $env:USERPROFILE "pip\pip.ini")
    (Join-Path $env:USERPROFILE "pip.ini")
    "C:\ProgramData\pip\pip.ini"
  )
}

# Strip NVIDIA NGC extra-index from user/site pip.ini (backup *.roseyes-bak). Permanent fix.
function Disable-RoseeyesNvidiaPipIndex {
  $changed = $false
  foreach ($path in Get-RoseeyesUserPipIniPaths) {
    if (-not (Test-Path $path)) { continue }
    $raw = Get-Content -Raw -Path $path
    if ($raw -match 'ROSEyes:\s*NVIDIA NGC') { continue }
    if ($raw -notmatch '(?m)^\s*https://pypi\.ngc\.nvidia\.com\s*$') { continue }

    $bak = "$path.roseyes-bak"
    if (-not (Test-Path $bak)) {
      Copy-Item -Path $path -Destination $bak -Force
    }
    # Keep useful flags; drop broken/slow NGC mirrors that fail DNS on this LAN.
    $clean = @"
# ROSEyes: NVIDIA NGC extra-index removed (backup next to this file).
[global]
no-cache-dir = true
index-url = https://pypi.org/simple
"@
    try {
      Set-Content -Path $path -Value $clean.TrimEnd() -Encoding ascii
      Write-Host "Cleaned NVIDIA pip index from $path (backup $bak)"
      $changed = $true
    } catch {
      Write-Warning "Could not write $path (need admin?): $_"
    }
  }
  if (-not $changed) {
    Write-Host "No NVIDIA NGC pip.ini found (already clean)"
  }
}

# Run a pip command against PyPI only (ignore NVIDIA / site pip.ini extra indexes).
function Invoke-RoseeyesPip {
  param(
    [Parameter(Mandatory = $true)][string]$Python,
    [Parameter(Mandatory = $true)][string[]]$PipArgs
  )
  Set-RoseeyesPythonIoUtf8
  $savedCfg = $env:PIP_CONFIG_FILE
  $savedExtra = $env:PIP_EXTRA_INDEX_URL
  $savedIndex = $env:PIP_INDEX_URL
  $savedTrusted = $env:PIP_TRUSTED_HOST
  $prevEap = $ErrorActionPreference
  $ErrorActionPreference = "Continue"
  try {
    # Windows null device: load no pip.ini (user/global NVIDIA NGC mirrors).
    $env:PIP_CONFIG_FILE = "nul"
    Remove-Item Env:\PIP_EXTRA_INDEX_URL -ErrorAction SilentlyContinue
    Remove-Item Env:\PIP_TRUSTED_HOST -ErrorAction SilentlyContinue
    $env:PIP_INDEX_URL = "https://pypi.org/simple"

    $sub = $PipArgs[0]
    $rest = @()
    if ($PipArgs.Count -gt 1) { $rest = $PipArgs[1..($PipArgs.Count - 1)] }
    $cli = @(
      $sub
      "--isolated"
      "--disable-pip-version-check"
      "--index-url", "https://pypi.org/simple"
    ) + $rest
    & $Python -m pip @cli 2>&1 | ForEach-Object { Write-Host ([string]$_) }
    return [int]$LASTEXITCODE
  } finally {
    $ErrorActionPreference = $prevEap
    if ($null -eq $savedCfg) {
      Remove-Item Env:\PIP_CONFIG_FILE -ErrorAction SilentlyContinue
    } else {
      $env:PIP_CONFIG_FILE = $savedCfg
    }
    if ($null -eq $savedExtra) {
      Remove-Item Env:\PIP_EXTRA_INDEX_URL -ErrorAction SilentlyContinue
    } else {
      $env:PIP_EXTRA_INDEX_URL = $savedExtra
    }
    if ($null -eq $savedIndex) {
      Remove-Item Env:\PIP_INDEX_URL -ErrorAction SilentlyContinue
    } else {
      $env:PIP_INDEX_URL = $savedIndex
    }
    if ($null -eq $savedTrusted) {
      Remove-Item Env:\PIP_TRUSTED_HOST -ErrorAction SilentlyContinue
    } else {
      $env:PIP_TRUSTED_HOST = $savedTrusted
    }
  }
}

# Best-effort: upgrade pip in the PlatformIO venv (kills the "new release of pip" spam).
function Update-RoseeyesPlatformIoPip {
  Disable-RoseeyesNvidiaPipIndex
  $python = Get-RoseeyesPlatformIoPython
  if (-not $python) {
    Write-Host "PlatformIO penv python not found - skip pip upgrade"
    return
  }
  Write-Host "Updating PlatformIO pip ($python)..."
  [void](Invoke-RoseeyesPip -Python $python -PipArgs @(
      "install", "-U", "pip"
    ))
}

# USB-CDC bootloader entry (Arduino IDE style).
function Invoke-Roseeyes1200BpsTouch {
  param([Parameter(Mandatory = $true)][string]$Port)
  try {
    $sp = New-Object System.IO.Ports.SerialPort $Port, 1200
    $sp.Open()
    Start-Sleep -Milliseconds 50
    $sp.Close()
    $sp.Dispose()
  } catch {
    Write-Warning "1200bps touch on $Port failed: $_"
  }
  Start-Sleep -Seconds 2
}

# Serial-upload a WSL-built image without Windows PlatformIO rebuild / click UTF-8 crash.
function Invoke-RoseeyesPrebuiltSerialUpload {
  param(
    [Parameter(Mandatory = $true)][string]$BuildDir,
    [Parameter(Mandatory = $true)][string]$Port,
    [int]$Baud = 460800,
    [switch]$ManualBootloader
  )

  Set-RoseeyesPythonIoUtf8
  $firmware = Join-Path $BuildDir "firmware.bin"
  $bootloader = Join-Path $BuildDir "bootloader.bin"
  $partitions = Join-Path $BuildDir "partitions.bin"
  foreach ($path in @($firmware, $bootloader, $partitions)) {
    if (-not (Test-Path $path)) {
      throw "Missing flash artifact: $path"
    }
  }

  $esptool = Get-RoseeyesEsptoolScript
  $python = Get-RoseeyesPlatformIoPython
  if (-not $python) { $python = "python" }

  Write-Host "Serial upload (prebuilt esptool, no PIO rebuild) to $Port @ $Baud"
  Write-Host "  build: $BuildDir"
  Write-Host "  esptool: $esptool"

  if ($ManualBootloader) {
    Write-Host "Manual bootloader: hold B, tap R, release B, then press Enter."
    [void](Read-Host)
  } else {
    Write-Host "Auto-reset: 1200bps touch + wait. Close Serial Monitor first."
    Invoke-Roseeyes1200BpsTouch -Port $Port
  }

  # Standard Arduino-ESP32 offsets for seeed_xiao_esp32s3.
  $args = @(
    $esptool
    "--chip", "esp32s3"
    "--port", $Port
    "--baud", "$Baud"
    "--before", "default_reset"
    "--after", "hard_reset"
    "write_flash", "-z"
    "--flash_mode", "dio"
    "--flash_freq", "80m"
    "--flash_size", "8MB"
    "0x0", $bootloader
    "0x8000", $partitions
    "0x10000", $firmware
  )

  $prevEap = $ErrorActionPreference
  $ErrorActionPreference = "Continue"
  try {
    $out = & $python @args 2>&1
    $code = [int]$LASTEXITCODE
  } finally {
    $ErrorActionPreference = $prevEap
  }
  foreach ($line in $out) {
    Write-Host ([string]$line)
  }
  if ($code -ne 0) {
    Write-Error "esptool serial upload failed (exit $code)"
  } else {
    Write-Host "Serial upload finished OK (exit 0)"
  }
  return $code
}

# OTA-upload a prebuilt firmware.bin without invoking PlatformIO rebuild
# (critical after WSL micro-ROS builds — Windows cannot rebuild libmicroros).
function Invoke-RoseeyesPrebuiltOtaUpload {
  param(
    [Parameter(Mandatory = $true)][string]$FirmwareBin,
    [Parameter(Mandatory = $true)][string]$OtaIp,
    [int]$OtaPort = 3232
  )

  Set-RoseeyesPythonIoUtf8
  if (-not (Test-Path $FirmwareBin)) {
    throw "Prebuilt firmware not found: $FirmwareBin"
  }

  $espota = Get-RoseeyesEspotaScript
  $python = Get-RoseeyesPlatformIoPython
  if (-not $python) { $python = "python" }

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
# For WSL-built micro-ROS images, prefer Invoke-RoseeyesPrebuilt* helpers instead.
function Invoke-RoseeyesUpload {
  param(
    [Parameter(Mandatory = $true)][string]$Pio,
    [Parameter(Mandatory = $true)][string]$ProjectDir,
    [Parameter(Mandatory = $true)][string]$EnvName,
    [string]$Port = "COM16",
    [string]$OtaIp = "",
    [switch]$ManualBootloader
  )

  Set-RoseeyesPythonIoUtf8
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

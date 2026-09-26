# ROSEyes flash / upload helpers (dot-sourced from Common.ps1).

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
# (critical after WSL micro-ROS builds â€” Windows cannot rebuild libmicroros).
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
  # terminating ErrorRecord when merged via 2>&1 â€” temporarily continue.
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

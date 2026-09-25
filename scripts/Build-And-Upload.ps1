<#
.SYNOPSIS
  Builds and uploads ROSEyes firmware (eyes-only by default, Windows-native).
.PARAMETER Port
  Serial port for upload (default COM16).
.PARAMETER OtaIp
  If set, upload via ArduinoOTA / espota to this device IP instead of serial.
.PARAMETER SkipUpload
  Build only.
.PARAMETER ManualBootloader
  Wait for Enter after you put the board in bootloader (B+R), then upload with --before=no_reset.
.PARAMETER FullMicroRos
  Build the micro-ROS env (requires WSL - prefer Build-And-Upload-Wsl.ps1).
.PARAMETER ListPorts
  List Espressif USB COM ports and exit.
#>
param(
  [string]$Port = "COM16",
  [string]$OtaIp = "",
  [switch]$SkipUpload,
  [switch]$ManualBootloader,
  [switch]$FullMicroRos,
  [switch]$ListPorts
)

$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
. (Join-Path $PSScriptRoot "Common.ps1")
. (Join-Path $PSScriptRoot "Resolve-PlatformIO.ps1")

if ($ListPorts) {
  $ports = @(Get-EspUsbSerialPorts)
  if ($ports.Count -eq 0) {
    Write-Host "No Espressif USB serial ports (VID_303A) found."
  } else {
    Write-Host "Espressif USB ports: $($ports -join ', ')"
  }
  exit 0
}

$Pio = Get-PlatformIoCommand
$EnvName = "xiao_esp32s3_eyes_only"
$FirmwareDir = Join-Path $RepoRoot "firmware"

if ($FullMicroRos) {
  Write-Warning "Native Windows cannot compile micro_ros_platformio. Use Build-And-Upload-Wsl.ps1."
  $EnvName = "xiao_esp32s3"
  [void](Set-RoseeyesSeedBuildFlags -RequireAgentIp -FirmwareIncludeDir (Join-Path $FirmwareDir "include"))
} else {
  [void](Set-RoseeyesSeedBuildFlags -FirmwareIncludeDir (Join-Path $FirmwareDir "include"))
}

Write-Host "Building env=$EnvName (project-dir=$FirmwareDir)"
Write-Host "Using PlatformIO: $Pio"
& $Pio run -d $FirmwareDir -e $EnvName
$buildExit = $LASTEXITCODE
if ($buildExit -ne 0) { exit $buildExit }

if ($SkipUpload) {
  Write-Host "SkipUpload set - build finished."
  exit 0
}

$uploadExit = Invoke-RoseeyesUpload `
  -Pio $Pio `
  -ProjectDir $FirmwareDir `
  -EnvName $EnvName `
  -Port $Port `
  -OtaIp $OtaIp `
  -ManualBootloader:$ManualBootloader

if ($uploadExit -is [array]) { $uploadExit = $uploadExit[-1] }
$uploadExit = [int]$uploadExit

if ($uploadExit -ne 0 -and -not $OtaIp) {
  Write-Host ""
  Write-Host "Serial upload failed (exit $uploadExit). Try:"
  Write-Host "  1) Close any Serial Monitor / pio device monitor"
  Write-Host "  2) .\scripts\Build-And-Upload.ps1 -ListPorts"
  Write-Host "  3) .\scripts\Build-And-Upload.ps1 -Port COMx -ManualBootloader"
  Write-Host "  4) After WiFi/OTA is up: .\scripts\Build-And-Upload.ps1 -OtaIp 192.168.x.x"
}
exit $uploadExit

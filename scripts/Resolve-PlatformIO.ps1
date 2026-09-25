<#
.SYNOPSIS
  Resolves the PlatformIO CLI (pio / platformio).
#>
function Get-PlatformIoCommand {
  $pio = Get-Command pio -ErrorAction SilentlyContinue
  if ($pio) { return $pio.Source }
  $platformio = Get-Command platformio -ErrorAction SilentlyContinue
  if ($platformio) { return $platformio.Source }
  $fallback = Join-Path $env:USERPROFILE ".platformio\penv\Scripts\platformio.exe"
  if (Test-Path $fallback) { return $fallback }
  throw "PlatformIO CLI not found. Install from https://platformio.org/ or ensure ~/.platformio/penv/Scripts is on PATH."
}

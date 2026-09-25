<#
.SYNOPSIS
  Lists Espressif (VID_303A) USB serial ports useful for XIAO upload.
#>
$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "Common.ps1")

Write-Host "All COM-looking PnP ports:"
Get-PnpDevice -Class Ports -Status OK -ErrorAction SilentlyContinue |
  ForEach-Object { Write-Host ("  - " + $_.FriendlyName) }

Write-Host ""
$esp = @(Get-EspUsbSerialPorts)
if ($esp.Count -eq 0) {
  Write-Host "No Espressif USB CDC ports found (VID_303A)."
  Write-Host "Plug the XIAO, or hold B / tap R to enter bootloader and re-check."
  exit 1
}

Write-Host "Espressif upload candidates: $($esp -join ', ')"
exit 0

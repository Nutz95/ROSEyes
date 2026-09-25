<#
.SYNOPSIS
  Runs firmware native unit tests and Python code-quality guardrails.
#>
$ErrorActionPreference = "Stop"
$RepoRoot = Split-Path -Parent $PSScriptRoot
$FirmwareDir = Join-Path $RepoRoot "firmware"
$GuardrailScript = Join-Path $RepoRoot "tests\guardrails\run_guardrails.py"
. (Join-Path $PSScriptRoot "Resolve-PlatformIO.ps1")
$Pio = Get-PlatformIoCommand

Write-Host "== Guardrails =="
python $GuardrailScript $RepoRoot
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "== Native unit tests (PlatformIO) =="
Write-Host "Using PlatformIO: $Pio"
Set-Location $FirmwareDir
& $Pio test -e native
exit $LASTEXITCODE

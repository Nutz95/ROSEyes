# ROSEyes pip helpers (dot-sourced from Common.ps1).

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

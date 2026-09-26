<#
.SYNOPSIS
  Builds and installs a native Windows MicroXRCEAgent (no Docker).
.DESCRIPTION
  Clones eProsima Micro-XRCE-DDS-Agent and builds an isolated Release install.
  Default install dir: MICROROS_AGENT_HOME, else sibling of ROS2_WINDOWS_SETUP,
  else %LOCALAPPDATA%\ROSEyes\MicroXRCEAgent.
.EXAMPLE
  .\scripts\Install-MicroRosAgent.ps1
#>
param(
  [string]$SourceDir = "",
  [string]$InstallDir = "",
  [string]$GitTag = "v2.4.3",
  [string]$VsDevShell = ""
)

$ErrorActionPreference = "Stop"
. (Join-Path $PSScriptRoot "Common.ps1")

if (-not $InstallDir -or $InstallDir.Trim() -eq "") {
  $InstallDir = Get-RoseeyesMicroXrceAgentHome
}
if (-not $SourceDir -or $SourceDir.Trim() -eq "") {
  $parent = Split-Path -Parent $InstallDir
  if (-not $parent) { $parent = $env:LOCALAPPDATA }
  $SourceDir = Join-Path $parent "Micro-XRCE-DDS-Agent"
}

function Find-VsDevShell {
  if ($VsDevShell -and (Test-Path $VsDevShell)) { return $VsDevShell }
  $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
  if (Test-Path $vswhere) {
    $installPath = & $vswhere -latest -products * `
      -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
      -property installationPath
    if ($installPath) {
      $candidate = Join-Path $installPath "Common7\Tools\Launch-VsDevShell.ps1"
      if (Test-Path $candidate) { return $candidate }
    }
  }
  foreach ($p in @(
      "${env:ProgramFiles}\Microsoft Visual Studio\2022\Community\Common7\Tools\Launch-VsDevShell.ps1",
      "${env:ProgramFiles}\Microsoft Visual Studio\2022\Professional\Common7\Tools\Launch-VsDevShell.ps1"
    )) {
    if (Test-Path $p) { return $p }
  }
  throw "Visual Studio C++ tools not found. Install VS with Desktop C++ workload."
}

if (-not (Get-Command cmake -ErrorAction SilentlyContinue)) {
  throw "cmake not found on PATH."
}
if (-not (Get-Command git -ErrorAction SilentlyContinue)) {
  throw "git not found on PATH."
}

$devShell = Find-VsDevShell
Write-Host "Using VS dev shell: $devShell"
. $devShell -Arch amd64 -HostArch amd64

if (-not (Test-Path $SourceDir)) {
  Write-Host "Cloning Micro-XRCE-DDS-Agent $GitTag -> $SourceDir"
  git clone --depth 1 --branch $GitTag https://github.com/eProsima/Micro-XRCE-DDS-Agent.git $SourceDir
  if ($LASTEXITCODE -ne 0) {
    throw "git clone failed ($LASTEXITCODE)"
  }
}

$buildDir = Join-Path $SourceDir "build-win"
New-Item -ItemType Directory -Force -Path $buildDir | Out-Null
New-Item -ItemType Directory -Force -Path $InstallDir | Out-Null

Push-Location $buildDir
try {
  Write-Host "Configuring (isolated Fast DDS deps; first run downloads a lot)..."
  cmake -G "Visual Studio 17 2022" -A x64 `
    -DCMAKE_INSTALL_PREFIX="$InstallDir" `
    -DUAGENT_SUPERBUILD=ON `
    -DUAGENT_ISOLATED_INSTALL=ON `
    -DUAGENT_USE_SYSTEM_FASTDDS=OFF `
    -DUAGENT_USE_SYSTEM_FASTCDR=OFF `
    -DUAGENT_P2P_PROFILE=OFF `
    -DUAGENT_BUILD_EXECUTABLE=ON `
    $SourceDir
  if ($LASTEXITCODE -ne 0) { throw "cmake configure failed ($LASTEXITCODE)" }

  Write-Host "Building Release (superbuild: deps + agent)..."
  cmake --build . --config Release --parallel
  if ($LASTEXITCODE -ne 0) { throw "cmake build failed ($LASTEXITCODE)" }

  # Superbuild installs into the prefix; also search the nested build tree.
  $exe = Get-ChildItem $InstallDir -Recurse -Filter "MicroXRCEAgent.exe" -ErrorAction SilentlyContinue |
    Select-Object -First 1
  if (-not $exe) {
    $exe = Get-ChildItem $buildDir -Recurse -Filter "MicroXRCEAgent.exe" -ErrorAction SilentlyContinue |
      Select-Object -First 1
  }
  if (-not $exe) {
    # Some layouts need an explicit install step on the inner project.
    $inner = Join-Path $buildDir "uagent\src\uagent-build"
    if (Test-Path $inner) {
      cmake --install $inner --config Release --prefix $InstallDir
    } else {
      cmake --install $buildDir --config Release --prefix $InstallDir
    }
    $exe = Get-ChildItem $InstallDir -Recurse -Filter "MicroXRCEAgent.exe" -ErrorAction SilentlyContinue |
      Select-Object -First 1
  }
  if (-not $exe) {
    throw "Build finished but MicroXRCEAgent.exe not found under $InstallDir or $buildDir"
  }

  # Copy exe (+ sibling DLLs) into a stable bin folder when found in the build tree.
  $binOut = Join-Path $InstallDir "bin"
  New-Item -ItemType Directory -Force -Path $binOut | Out-Null
  Copy-Item $exe.FullName (Join-Path $binOut "MicroXRCEAgent.exe") -Force
  Get-ChildItem $exe.DirectoryName -Filter "*.dll" -ErrorAction SilentlyContinue |
    ForEach-Object { Copy-Item $_.FullName $binOut -Force }
} finally {
  Pop-Location
}

$finalExe = Join-Path $InstallDir "bin\MicroXRCEAgent.exe"
if (-not (Test-Path $finalExe)) {
  throw "Expected $finalExe after install copy"
}

$env:MICROROS_AGENT_EXE = $finalExe
Write-Host "Installed: $finalExe"
Write-Host "Set permanently (optional):"
Write-Host "  [System.Environment]::SetEnvironmentVariable('MICROROS_AGENT_EXE','$finalExe','User')"
Write-Host "Then: .\scripts\Start-MicroRosAgent.ps1"

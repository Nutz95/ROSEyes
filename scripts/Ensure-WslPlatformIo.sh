#!/usr/bin/env bash
# Ensures PlatformIO Core is available in WSL with ~/.platformio/penv
# (required by micro_ros_platformio, which sources that venv).
set -euo pipefail

# Do not put a half-created penv ahead of system python during bootstrap.
export PATH="$HOME/.local/bin:/usr/bin:$PATH"
PYTHON="${ROSEYES_PYTHON:-/usr/bin/python3}"
PIO_VENV="$HOME/.platformio/penv"
PIO_BIN="$PIO_VENV/bin/pio"

install_pip_via_get_pip() {
  local get_pip="/tmp/roseyes-get-pip.py"
  echo "Trying get-pip.py --user (no sudo) ..."
  if command -v curl >/dev/null 2>&1; then
    curl -fsSL https://bootstrap.pypa.io/get-pip.py -o "$get_pip"
  elif command -v wget >/dev/null 2>&1; then
    wget -qO "$get_pip" https://bootstrap.pypa.io/get-pip.py
  else
    echo "curl/wget missing — cannot fetch get-pip.py"
    return 1
  fi
  # Force system interpreter so a broken penv on PATH cannot mark --user illegal.
  "$PYTHON" "$get_pip" --user
  rm -f "$get_pip"
  "$PYTHON" -m pip --version >/dev/null 2>&1
}

install_pip_via_apt() {
  echo "Installing python3-pip / python3-venv via apt (non-interactive sudo) ..."
  if ! command -v sudo >/dev/null 2>&1; then
    echo "ERROR: sudo not available." >&2
    return 1
  fi
  if ! sudo -n true >/dev/null 2>&1; then
    echo "ERROR: sudo needs a password (non-interactive session)." >&2
    return 1
  fi
  sudo -n apt-get update -y
  sudo -n apt-get install -y python3-pip python3-venv
  "$PYTHON" -m pip --version >/dev/null 2>&1
}

ensure_pip() {
  if "$PYTHON" -m pip --version >/dev/null 2>&1; then
    return 0
  fi

  echo "pip missing — trying ensurepip --user ..."
  if "$PYTHON" -m ensurepip --user >/dev/null 2>&1; then
    if "$PYTHON" -m pip --version >/dev/null 2>&1; then
      return 0
    fi
  fi

  if install_pip_via_get_pip; then
    return 0
  fi

  if install_pip_via_apt; then
    return 0
  fi

  echo "ERROR: could not install pip automatically." >&2
  echo "In a WSL terminal run once:" >&2
  echo "  sudo apt-get update && sudo apt-get install -y python3-pip python3-venv" >&2
  echo "Then re-run: .\\scripts\\Build-And-Upload-Wsl.ps1 -SkipUpload" >&2
  exit 1
}

download_get_pip() {
  local get_pip="$1"
  if command -v curl >/dev/null 2>&1; then
    curl -fsSL https://bootstrap.pypa.io/get-pip.py -o "$get_pip"
  elif command -v wget >/dev/null 2>&1; then
    wget -qO "$get_pip" https://bootstrap.pypa.io/get-pip.py
  else
    return 1
  fi
}

create_pio_venv() {
  mkdir -p "$HOME/.platformio"
  if [[ -e "$PIO_VENV" ]]; then
    echo "Removing incomplete PlatformIO venv at $PIO_VENV ..."
    rm -rf "$PIO_VENV"
  fi

  # Prefer virtualenv (bundles pip) — Debian system venv often lacks ensurepip.
  if "$PYTHON" -m pip install --user -U virtualenv && \
     "$PYTHON" -m virtualenv "$PIO_VENV"; then
    return 0
  fi

  # Fallback: empty venv then get-pip.py into it.
  if "$PYTHON" -c "import venv" >/dev/null 2>&1; then
    "$PYTHON" -m venv --without-pip "$PIO_VENV"
    local get_pip="/tmp/roseyes-get-pip.py"
    download_get_pip "$get_pip"
    "$PIO_VENV/bin/python3" "$get_pip"
    rm -f "$get_pip"
    return 0
  fi

  echo "ERROR: could not create $PIO_VENV" >&2
  echo "Run in WSL: sudo apt-get install -y python3-venv python3-pip" >&2
  exit 1
}

ensure_cmake() {
  export PATH="$HOME/.platformio/penv/bin:$HOME/.local/bin:$PATH"
  if command -v cmake >/dev/null 2>&1; then
    return 0
  fi
  if [[ ! -x "$PIO_BIN" ]]; then
    echo "ERROR: cannot install cmake without PlatformIO venv." >&2
    return 1
  fi
  echo "cmake missing — installing into PlatformIO venv ..."
  # shellcheck disable=SC1091
  source "$PIO_VENV/bin/activate"
  python -m pip install -U cmake
  deactivate
  if [[ -x "$PIO_VENV/bin/cmake" ]]; then
    mkdir -p "$HOME/.local/bin"
    ln -sfn "$PIO_VENV/bin/cmake" "$HOME/.local/bin/cmake"
  fi
  export PATH="$HOME/.platformio/penv/bin:$HOME/.local/bin:$PATH"
  if ! command -v cmake >/dev/null 2>&1; then
    echo "ERROR: cmake still not on PATH." >&2
    echo "Install manually in WSL: sudo apt-get install -y cmake" >&2
    return 1
  fi
}

if [[ -x "$PIO_BIN" ]]; then
  ensure_cmake
  echo "PlatformIO already available: $PIO_BIN (cmake=$(command -v cmake))"
  exit 0
fi

ensure_pip

echo "Creating PlatformIO venv at $PIO_VENV (needed by micro-ROS) ..."
create_pio_venv
# shellcheck disable=SC1091
source "$PIO_VENV/bin/activate"
python -m pip install -U pip
python -m pip install -U platformio
deactivate

export PATH="$HOME/.platformio/penv/bin:$PATH"
if [[ ! -x "$PIO_BIN" ]]; then
  echo "ERROR: pio not found at $PIO_BIN after install." >&2
  exit 1
fi

mkdir -p "$HOME/.local/bin"
ln -sfn "$PIO_BIN" "$HOME/.local/bin/pio"
ensure_cmake

echo "PlatformIO ready: $PIO_BIN (cmake=$(command -v cmake))"

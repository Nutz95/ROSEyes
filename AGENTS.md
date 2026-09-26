# AGENTS.md — working on ROSEyes

Guidance for coding agents and human contributors.

## Do

- Keep **1 class / enum class / struct per header** under `firmware/include`.
- Keep each class file **under 400 lines** and **under 30 public methods**.
- Document every **public** method (including constructors) with `/** ... */`.
- Prefer meaningful identifiers; short names allowed only for `x/y/z/w/h` and similar geometry locals on the allowlist in `tests/guardrails/run_guardrails.py`.
- Put secrets only in environment variables (`WIFI_SSID`, `WIFI_PASS`, `MICROROS_AGENT_IP`). Never commit credentials.
- Set `ROS2_WINDOWS_SETUP` to your ROS 2 `setup.ps1` (or pass `-RosSetup` to `Activate-Ros.ps1` / `Test-Ros.ps1`).
- Run `scripts/Run-Tests.ps1` after C++ changes.
- Leave **git commits** to the human unless they explicitly ask.

## Don’t

- Drive the TOF sensor until a dedicated task asks for it (stub only).
- Put multiple types in one header/source file.
- Add inline imports in Python (imports stay at module top).
- Edit the Cursor plan file under `.cursor/plans/`.

## Common commands

```powershell
# Host Python (ROS + pygame/Pillow) — run once / when requirements change
.\scripts\0_setup_python_env.ps1

# Quality + native unit tests
.\scripts\Run-Tests.ps1

# Flash eyes-only (Windows-native, no micro-ROS lib build)
.\scripts\Build-And-Upload.ps1 -Port COM16
.\scripts\Build-And-Upload.ps1 -Port COM16 -ManualBootloader
.\scripts\Build-And-Upload.ps1 -OtaIp 192.168.20.199

# Flash full micro-ROS (WSL2 required for libmicroros)
.\scripts\Build-And-Upload-Wsl.ps1 -Port COM16
.\scripts\Build-And-Upload-Wsl.ps1 -OtaIp 192.168.20.199

# micro-ROS agent (native Windows MicroXRCEAgent; first install once)
.\scripts\Install-MicroRosAgent.ps1
.\scripts\Start-MicroRosAgent.ps1

# ROS env + sample gaze publish
.\scripts\Test-Ros.ps1

# Xbox → eyes (host ROS + pygame; no Docker)
.\scripts\Start-XboxGaze.ps1

# Interactive host ROS menu (debug cockpit, Xbox, mode, echoes)
.\scripts\Start-RosEyes.ps1
```

## Firmware notes

- Board: `seeed_xiao_esp32s3`
- Envs: `xiao_esp32s3_eyes_only` (default), `xiao_esp32s3` (micro-ROS), `native` (tests)
- micro-ROS: `micro_ros_platformio`, transport `wifi`, distro `jazzy` — **build on WSL**, not cmd.exe
- Seed macros: `WIFI_SSID_SEED`, `WIFI_PASS_SEED`, `MICROROS_AGENT_IP_SEED`, `MICROROS_AGENT_PORT_SEED` via `PLATFORMIO_BUILD_FLAGS`
- Entry point: `firmware/src/main.cpp` → `EyeApplication`

## Review bar

Before handing off large changes, run a thermo-nuclear style code-quality pass: look for spaghetti branches, files approaching 1k lines, leaky abstractions, and missed simplifications — not only “it compiles”.

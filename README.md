# ROSEyes

ROS 2–compatible dual “eye” displays for mobile robots, driven by a **Seeed XIAO ESP32-S3 Sense** and controlled over **micro-ROS (WiFi / UDP)**.

Default behavior: cartoon eyes with a gentle idle gaze and a semi-random blink every 2–3 seconds.  
Remote control: publish normalized gaze on `/eyes/gaze` (`geometry_msgs/Vector3`, `x`/`y` in **[-1, 1]**).

| Piece | Product |
|-------|---------|
| MCU | [XIAO ESP32-S3 Sense](https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html) |
| Base | [Grove Base for XIAO](https://www.seeedstudio.com/Grove-Shield-for-Seeeduino-XIAO-p-4621.html) |
| Displays | [Spotpear Electronic EYE 0.71″ dual LCD (GC9D01)](https://spotpear.com/shop/Raspberry-Pi-ESP32-Pico-Arduino-STM32-51-0.71-inch-Round-LCD-EYE-Double.html) |
| Range (future) | [Waveshare TOF Laser Range Sensor Mini](https://www.waveshare.com/tof-laser-range-sensor-mini.htm) |

Architecture details: [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)  
Agent / contributor guide: [AGENTS.md](AGENTS.md)

---

## Features

- Dual 160×160 GC9D01 SPI eyes (modular C++ firmware, PlatformIO)
- Idle animation + blink without any ROS traffic
- micro-ROS over WiFi (client **jazzy**, agent Docker `microros/micro-ros-agent:jazzy`)
- WiFi / agent credentials seeded at flash time from env vars into **ESP32 NVS**
- Host tools: PowerShell scripts, Xbox → ROS publisher (pygame)
- Native unit tests + source guardrails (1 class / file, size, docs)

TOF I2C is wired and stubbed (`TofRangeSensorStub`) but **not** driven yet.

---

## Repository layout

```
firmware/     PlatformIO project (XIAO + native tests)
scripts/      PowerShell helpers (ROS, flash, tests, agent)
python/       Xbox gaze publisher
docker/       micro-ROS agent compose file
docs/         Architecture
tests/        Guardrail scripts
```

---

## Hardware wiring

### Eyes (SPI)

| Signal | XIAO | GPIO |
|--------|------|------|
| RST1 | D0 | 1 |
| RST2 | D1 | 2 |
| CS1 | D2 | 3 |
| CS2 | D3 | 4 |
| CLK | D8 | 7 |
| DC | D9 | 8 |
| SDA (MOSI) | D10 | 9 |
| 3V3 | 3V3 | — |
| GND | GND | — |

If panels stay black after a successful flash, tie **BL1/BL2** (backlight) to **3V3** (module-dependent).

### TOF (I2C, reserved)

| Signal | XIAO | GPIO |
|--------|------|------|
| SDA | D4 | 5 |
| SCL | D5 | 6 |
| 5V | 5V | — |
| GND | GND | — |

---

## Prerequisites

1. **Python 3** with [PlatformIO](https://platformio.org/): `pip install platformio`
2. **ROS 2 lyrical** on Windows (this machine: `I:\ROS\ros2-windows`)
3. **Docker Desktop** (for `micro-ros-agent:jazzy`)
4. **WSL2** (required only for the full micro-ROS firmware env — bash/colcon)
5. Environment variables (for micro-ROS flashing):
   - `WIFI_SSID`
   - `WIFI_PASS`
   - `MICROROS_AGENT_IP` (PC LAN IP reachable from the ESP; auto-detected if unset)
   - optional `MICROROS_AGENT_PORT` (default `8888`)
6. Optional Xbox path: `pip install -r python/requirements.txt` inside an env that also has `rclpy`

---

## Quick start

### 1. Unit tests + guardrails

```powershell
cd I:\GIT\ROSEyes
powershell -ExecutionPolicy Bypass -File .\scripts\Run-Tests.ps1
```

### 2. Start micro-ROS agent

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\Start-MicroRosAgent.ps1
```

Uses [docker/docker-compose.agent.yml](docker/docker-compose.agent.yml) (`udp4 --port 8888`).

### 3. Flash the ESP (COM16 by default)

**Eyes-only (Windows-native, recommended first bring-up):**

```powershell
# Optional but recommended: seed WiFi into NVS so OTA works after boot
# ($env:WIFI_SSID / WIFI_PASS already set on this machine)

.\scripts\List-SerialPorts.ps1
# Normal flash (no buttons) — same auto-reset path as Arduino IDE
.\scripts\Build-And-Upload.ps1 -Port COM16
# Fallback only if CDC glitches:
.\scripts\Build-And-Upload.ps1 -Port COM16 -ManualBootloader

# Later, once serial log shows "OTA: ready at x.x.x.x":
.\scripts\Build-And-Upload.ps1 -OtaIp 192.168.x.x
```

**Full micro-ROS (requires WSL2 — `micro_ros_platformio` needs bash/colcon):**

```powershell
.\scripts\Build-And-Upload-Wsl.ps1 -Port COM16
.\scripts\Build-And-Upload-Wsl.ps1 -OtaIp 192.168.x.x
```

### 4. Activate ROS and publish a test gaze

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\Test-Ros.ps1 -GazeX 0.7 -GazeY -0.2
```

Or manually:

```powershell
. .\scripts\Activate-Ros.ps1
python .\python\xbox_gaze_publisher.py --invert-y
```

---

## ROS topics

| Topic | Type | Direction | Meaning |
|-------|------|-----------|---------|
| `eyes/gaze` | `geometry_msgs/msg/Vector3` | host → ESP | `x`/`y` in [-1, 1] (look right / down) |
| `eyes/blink` | `std_msgs/msg/Empty` | host → ESP | force one blink |
| `eyes/status` | `std_msgs/msg/String` | ESP → host | heartbeat / debug |

Gaze timeout: if no `eyes/gaze` for ~500 ms, idle animation resumes.

---

## Windows / Docker note

Docker Desktop does not provide Linux-style `--net=host`. The agent publishes UDP `8888` with `-p 8888:8888/udp`. Host `ros2` / `rclpy` (lyrical) may not always share DDS discovery with the jazzy agent container. If `ros2 topic list` does not show eye topics:

1. Confirm the agent container is running and ESP serial logs show WiFi + entity creation.
2. Use a ROS CLI container sharing the agent network (see `Test-Ros.ps1` fallback message).
3. Prefer running the Xbox publisher on the same ROS graph the agent exposes.

The ESP still speaks real micro-ROS XRCE-DDS to the agent regardless.

---

## License

MIT — see [LICENSE](LICENSE).

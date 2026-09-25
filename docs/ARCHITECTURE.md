# ROSEyes architecture

## System context

```mermaid
flowchart LR
  Xbox[Xbox_pygame_publisher]
  MaixCam[MaixCam_ball_tracker_future]
  HostRos[ROS2_lyrical_host]
  Agent[micro_ros_agent_Docker_jazzy_UDP8888]
  Esp[XIAO_ESP32S3_Sense]
  Eyes[Dual_GC9D01]
  Nvs[NVS_credentials]
  Tof[TOF_Mini_stub]

  Xbox -->|eyes/gaze| HostRos
  MaixCam -->|eyes/gaze| HostRos
  HostRos --> Agent
  Agent <-->|XRCE_DDS_UDP| Esp
  Esp --> Eyes
  Esp --> Nvs
  Esp -.-> Tof
```

## Firmware modules

Two PlatformIO device envs:

- `xiao_esp32s3_eyes_only` — display + idle/blink; builds on native Windows
- `xiao_esp32s3` — adds micro-ROS WiFi client; **must be built under WSL2** because `micro_ros_platformio` invokes bash/`colcon` to compile `libmicroros`

```mermaid
flowchart TB
  main[main.cpp]
  app[EyeApplication]
  display[DualEyeDisplay]
  driver[Gc9d01Driver]
  renderer[EyeRenderer]
  gaze[GazeState]
  blink[BlinkScheduler]
  idle[IdleEyeBehavior]
  rosNode[MicroRosEyeNode]
  store[WifiCredentialStore]
  nvs[NvsKeyValueStore]

  main --> app
  app --> display
  display --> driver
  app --> renderer
  renderer --> display
  app --> gaze
  app --> blink
  app --> idle
  app --> rosNode
  app --> store
  store --> nvs
  rosNode --> gaze
```

### Responsibility split

| Module | Responsibility |
|--------|----------------|
| `Gc9d01Driver` | Panel init + SPI command/data |
| `DualEyeDisplay` | Two CS/RST panels, fill primitives |
| `EyeRenderer` | Cartoon eye drawing / blink frame |
| `GazeState` | Normalized [-1,1] ↔ pixel offsets |
| `BlinkScheduler` | Random 2–3 s blink timing |
| `IdleEyeBehavior` | Slow left/right idle when ROS silent |
| `WifiCredentialStore` | NVS load/seed/save (`loadOrSeed`) |
| `MicroRosEyeNode` | WiFi transport, sub/pub, atomic entity lifecycle |
| `EyeApplication` | Orchestration + dirty-state rendering |
| `TofRangeSensorStub` | Reserved API for later I2C TOF |

Design rules: **1 class = 1 header/source pair**, files **&lt; 400 lines**, **&lt; 30 public methods**, documented public APIs. Enforced by `tests/guardrails/run_guardrails.py`.

## Boot / credential flow

```mermaid
sequenceDiagram
  participant Flash as Build_And_Upload
  participant Esp as Firmware
  participant Nvs as NVS
  participant Ap as WiFi_AP
  participant Agent as micro_ros_agent

  Flash->>Esp: firmware + WIFI_* / AGENT_IP seeds
  Esp->>Nvs: load credentials
  alt missing in NVS
    Esp->>Nvs: save seeds from build flags
  end
  Esp->>Ap: WiFi.begin via micro-ROS transport helper
  Esp->>Agent: XRCE session UDP 8888
  Agent-->>Esp: create participant / topics
```

## ROS contracts

- **`eyes/gaze`** (`geometry_msgs/msg/Vector3`)
  - `x`: look left (−1) … right (+1)
  - `y`: look up (−1) … down (+1) in display space (Xbox publisher inverts stick Y by default)
  - `z`: unused (0)
  - Freshness window: 500 ms
- **`eyes/blink`** (`std_msgs/msg/Empty`): one-shot blink request
- **`eyes/status`** (`std_msgs/msg/String`): ~1 Hz heartbeat

micro-ROS client distro: **jazzy** (`board_microros_distro`).  
Host tools on this workspace target **ROS 2 lyrical** (`I:\ROS\ros2-windows`).

## Future work

1. Implement `TofRangeSensorStub` → real Waveshare TOF Mini I2C driver; optionally publish `/eyes/range`.
2. MaixCam ball tracker publishes `/eyes/gaze` on the robot ROS graph.
3. Optional emotion / animation bank from Spotpear demos if RAM allows.
4. Camera on XIAO Sense (not used in v1).

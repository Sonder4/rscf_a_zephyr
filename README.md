# RSCF A Zephyr

`RSCF A Zephyr` is the standalone Zephyr RTOS migration repository for the
STM32F427IIH6-based RSCF A controller.

## Scope

- Zephyr baseline: `v4.3.0`
- Workspace tool: `west`
- Standard toolchain: `gnuarmemb` via STM32CubeCLT or `Zephyr SDK`
- Target board: `rscf_a_f427iih6`
- Migration style: standalone repository, not an in-place FreeRTOS rewrite

## Repository Layout

- `app/`: Zephyr application entrypoint and Kconfig integration
- `boards/arm/rscf_a_f427iih6/`: custom board definition
- `drivers/rscf/`: RSCF board-facing driver adapters
- `modules/rscf/`: RSCF services and device modules
- `profiles/`: static application profile options
- `scripts/bootstrap/`: workspace and toolchain bootstrap helpers
- `dts/bindings/rscf/`: custom devicetree bindings
- `docs/`: migration and bring-up documentation
- `artifacts/`: locally retained firmware outputs copied out of transient build dirs
- `ros2_ws/`: host-side ROS 2 workspace for MCU communication and tooling

## Quick Start

```bash
./scripts/bootstrap/bootstrap.sh
source .venv/bin/activate
cd .workspace
west build -b rscf_a_f427iih6 ../app
```

Host ROS 2 workspace:

```bash
source /opt/ros/humble/setup.bash
source /home/xuan/microros_ws/agent_ws/install/setup.bash
cd /home/xuan/RC2026/STM32/rscf_a_zephyr/ros2_ws
colcon build
source install/setup.bash
ros2 launch rscf_a_host rscf_host_io.launch.py start_agent:=true serial_device:=/dev/ttyACM0
```

## Current Status

This repository currently provides:

- the workspace manifest and bootstrap flow
- the custom board, DTS bindings, and USB CDC ACM overlay for `STM32F427IIH6`
- Zephyr-side BSP adapters for `CAN`, `USART`, `RTT log`, `DWT`, `LED`, `buzzer`, and external UART mux
- migrated RSCF services for `comm`, `chassis`, `motor`, `robot`, `event bus`, `daemon`, `health`, `debug fault`, `ROS bridge`
- migrated device paths for `Saber IMU UART` and `DTOF2510`
- migrated compatibility modules for `DJI/DM/servo motor`, `HC05`, `AD7190`, `mhall`, `OLED`, `master_process`, `remote_control`, `robot`, and `robot_cmd`
- integrated `mcu_comm` code generation path and a repo-local host ROS 2 workspace
- vendored `CMSIS-DSP` source integration replacing the previous local `arm_math` compatibility layer
- MCU-side `micro-ROS` over USB CDC ACM with host-side monitor/command nodes in `ros2_ws/`

Remaining work is mainly in hardware-depth validation and non-skeleton APP roles:

- `AD7190 / HC05 / mhall / OLED / remote / master_process` are integrated, but still need full real-hardware verification
- `arm / gimbal / BT` are currently task and state-machine skeletons, not complete actuator applications

## User Manual

See [docs/zephyr-user-manual.md](docs/zephyr-user-manual.md) for:

- current migration coverage and known gaps
- how Zephyr-side drivers are enabled in this repository
- how to add or disable a driver
- how to organize task logic with Zephyr-native patterns
- how `mcu_comm`, ROS2, and the app profile are connected

## Bootstrap Model

`./scripts/bootstrap/bootstrap.sh` creates a repo-local Zephyr workspace under
`.workspace/` using the official `zephyr v4.3.0` manifest. This avoids turning
the repository parent directory into a west workspace while still allowing this
repository to supply the out-of-tree board, DTS bindings, drivers, and app
sources.

The repository root is the source of truth for all application and migration
code. `.workspace/manifest` exists only as a west-managed manifest checkout and
should not be treated as an editable source tree.

If build directories under `.workspace/` are cleaned, the latest manually
retained firmware images are kept under `artifacts/microros/`.

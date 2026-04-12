# Getting Started

## 1. Prerequisites

- Linux shell environment
- `python3.12+`, or `uv` available to provision a local Python `3.12` runtime
- `cmake`
- network access for `west update`
- one supported toolchain:
  - installed `Zephyr SDK`, exported with `ZEPHYR_SDK_INSTALL_DIR`
  - or `gnuarmemb`, for example STM32CubeCLT exported with `ZEPHYR_TOOLCHAIN_VARIANT=gnuarmemb` and `GNUARMEMB_TOOLCHAIN_PATH`

## 2. Bootstrap

```bash
cd /home/xuan/RC2026/STM32/rscf_a_zephyr
./scripts/bootstrap/bootstrap.sh
source .venv/bin/activate
cd .workspace
```

The bootstrap flow performs these actions:

1. create `.venv/` with Python `3.12+`
2. install `west`
3. initialize `.workspace/` as a repo-local west workspace from the official
   `zephyr v4.3.0` manifest
4. fetch `zephyr`, `cmsis`, and `hal_stm32`
5. export the Zephyr CMake package
6. run `scripts/bootstrap/check_env.py`

## 3. Build

```bash
west build -b rscf_a_f427iih6 ../app
```

## 4. Flash

Current baseline runner configuration uses J-Link:

```bash
west flash
```

## 5. Baseline Validation

The first milestone validation is:

- successful `west build`
- UART console boot log
- status LED heartbeat path available
- board definition resolves all mandatory Zephyr metadata

## 6. Host ROS 2 Workspace

The repository also contains a host-side ROS 2 workspace at `ros2_ws/`.

Build it with:

```bash
source /opt/ros/humble/setup.bash
source /home/xuan/microros_ws/agent_ws/install/setup.bash
cd /home/xuan/RC2026/STM32/rscf_a_zephyr/ros2_ws
colcon build
source install/setup.bash
```

Start the host I/O path with:

```bash
ros2 launch rscf_a_host rscf_host_io.launch.py start_agent:=true serial_device:=/dev/ttyACM0
```

This launch file can:

- start the `micro_ros_agent` wrapper
- run `rscf_monitor` for telemetry summary
- optionally run `rscf_command_profile` for `/cmd_vel` and heading injection

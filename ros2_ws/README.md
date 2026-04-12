# RSCF A ROS 2 Workspace

`ros2_ws/` is the host-side ROS 2 workspace for communicating with the
Zephyr-based RSCF A controller over USB CDC ACM + micro-ROS agent.

## Layout

- `src/rscf_a_host/`: host package for agent startup, command publishing, and telemetry monitoring

## Build

```bash
cd /home/xuan/RC2026/STM32/rscf_a_zephyr/ros2_ws
source /opt/ros/humble/setup.bash
colcon build
```

## Runtime

If the micro-ROS agent was built in `/home/xuan/microros_ws/agent_ws`, source it
before launching:

```bash
source /opt/ros/humble/setup.bash
source /home/xuan/microros_ws/agent_ws/install/setup.bash
source /home/xuan/RC2026/STM32/rscf_a_zephyr/ros2_ws/install/setup.bash
ros2 launch rscf_a_host rscf_host_io.launch.py start_agent:=true serial_device:=/dev/ttyACM0
```

The launch file starts:

- an optional `micro_ros_agent` wrapper for the board USB CDC device
- `rscf_monitor` for telemetry summary and status-word decode
- an optional `rscf_command_profile` node for publishing `/cmd_vel` and heading targets

## Direct Node Usage

Monitor only:

```bash
ros2 run rscf_a_host rscf_monitor
```

Publish a constant command profile:

```bash
ros2 run rscf_a_host rscf_command_profile --ros-args \
  -p mode:=step \
  -p linear_x:=0.35 \
  -p angular_z:=0.25
```

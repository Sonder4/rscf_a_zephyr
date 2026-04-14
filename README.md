# RSCF A Zephyr

`rscf_a_zephyr` 是 `RSCF_A` 在 `STM32F427IIH6` 平台上的 Zephyr 版本工程，用于替代原 FreeRTOS 版本的板级、驱动、服务和主机通信主链。

## 项目状态

| 项目 | 说明 |
|------|------|
| 目标板卡 | `RSCF A` |
| 目标 MCU | `STM32F427IIH6` |
| Zephyr 基线 | `v4.3.0` |
| 默认工具链 | `gnuarmemb` |
| 默认通信主链 | `USB CDC ACM + RSCF Link vNext + rscf_link_bridge` |
| 兼容链路 | `mcu_comm` 代码生成、`micro-ROS` 兼容后端 |
| 板级硬件快照 | `docs/ioc_json/*.json` |

## 已接入内容

- 自定义开发板：`boards/arm/rscf_a_f427iih6/`
- 板级驱动：`rscf_led_status`、`rscf_buzzer`、`rscf_power_switch`、`rscf_ext_uart_mux`
- BSP 兼容层：`bsp_can`、`bsp_usart`、`bsp_log`、`bsp_dwt`、`bsp_spi`、`bsp_pwm`、`bsp_tim`
- 系统服务：`rscf_event_bus`、`rscf_daemon_service`、`rscf_health_service`、`rscf_comm_service`、`rscf_chassis_service`、`rscf_motor_service`、`rscf_robot_service`、`rscf_ros_bridge`、`rscf_debug_fault`
- 设备链路：`Saber C3 IMU`、`DTOF2510`
- 电机链路：`DJI Motor`、`DM Motor`、PWM `Servo Motor`
- 数学库：内置 `CMSIS-DSP`
- 主机侧 ROS 2：`ros2_ws/src/rscf_link_bridge`
- CI：单元测试、固件构建、ROS 2 工作空间构建

## 未完成闭环

以下内容已进入仓库，但尚未完成与旧工程等价的实机闭环：

- APP 角色：`arm`、`gimbal`、`BT`、行为树 / 高层状态机
- 兼容模块：`AD7190`、`HC05`、`mhall`、`remote_control`、`master_process / seasky_protocol`、`OLED`
- 舵机：`bus servo`
- 整机编排：`robot.c / robot_task.h / robot_cmd.c` 仍未达到旧工程完整任务拓扑

## 默认通信方案

- MCU 侧：`RSCF Link vNext runtime`
- 物理链路：`USB CDC ACM`
- Host 侧：`ros2_ws/src/rscf_link_bridge`
- 多 MCU 侧链路：`peer link (UART/CAN/SPI skeleton)`

默认 ROS 2 主题：

- 下行：`/cmd_vel`
- 上行：`/rscf/system_status`
- 上行：`/odom`
- 上行：`/imu/data`

`rscf_link_bridge` 当前提供以下参数：

- `device_path`
- `baudrate`
- `compat_mode`

`compat_mode` 默认值仍为 `true`。该参数只用于 bridge skeleton 阶段保留旧状态字和旧负载形态，不表示默认后端切回 `micro-ROS`。`micro-ROS` 仍作为兼容后端保留。

## 快速开始

### 1. 初始化环境

```bash
git clone git@github.com:Sonder4/rscf_a_zephyr.git
cd rscf_a_zephyr
./scripts/bootstrap/bootstrap.sh
source .venv/bin/activate
cd .workspace
```

### 2. 构建固件

```bash
west build -b rscf_a_f427iih6 ../app -d build_vnext
```

主要产物：

- `.workspace/build_vnext/zephyr/zephyr.elf`
- `.workspace/build_vnext/zephyr/zephyr.hex`

### 3. 烧录固件

```bash
west flash
```

### 4. 启动主机侧 ROS 2

```bash
source /opt/ros/humble/setup.bash
cd ros2_ws
colcon build
source install/setup.bash
ros2 launch rscf_link_bridge rscf_link_bridge.launch.py device_path:=/dev/ttyACM0 baudrate:=115200 compat_mode:=true
```

## 构建约束

- `docs/ioc_json/*.json` 保存旧工程 `.ioc` 的硬件快照
- `scripts/ioc/validate_board_from_ioc.py` 在构建期校验 Zephyr 板级定义与 `.ioc` 快照
- `POWER1 ~ POWER4`、`TIM8 PWM`、`TIM12/PH6 buzzer` 已收口到板级节点
- `I2C2` 与 `OLED` 当前未进入主线实机支持范围

## 仓库结构

- `app/`：Zephyr 应用入口、配置与 overlay
- `boards/arm/rscf_a_f427iih6/`：自定义开发板
- `drivers/bsp/`：旧 BSP 接口兼容层
- `drivers/rscf/`：板级设备驱动
- `modules/rscf/`：核心、设备与服务层
- `modules/compat/`：旧工程兼容模块
- `modules/motor/`：电机实现
- `generated/mcu_comm/`：生成后的兼容通信镜像
- `ros2_ws/`：主机侧 ROS 2 工作空间
- `third_party/cmsis_dsp/`：内置 `CMSIS-DSP`
- `.workspace/`：repo-local west 工作空间，不作为业务源码真源

## 文档

- [docs/README_CN.md](docs/README_CN.md)
- [docs/getting-started.md](docs/getting-started.md)
- [docs/zephyr-user-manual.md](docs/zephyr-user-manual.md)
- [docs/reference/rscf_a_hardware_map.md](docs/reference/rscf_a_hardware_map.md)
- [docs/ioc_json](docs/ioc_json)

## CI

GitHub Actions 工作流 `ci` 提供以下检查：

- `unit-tests`
- `firmware-build`
- `ros2-workspace-build`

触发方式：

- 推送到 `main`
- `pull_request`
- 手动触发 `workflow_dispatch`

## 已知限制

- `TIM2` 仍保留在 `bsp_tim` 兼容层，不拆成互斥的多个 Zephyr 子节点
- 兼容层接入不等于实机闭环完成
- `micro-ROS` 为兼容后端，不再是默认启动链路

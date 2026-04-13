# RSCF A Zephyr 移植仓库

`rscf_a_zephyr` 是 `RSCF_A` 原 STM32F427 FreeRTOS 工程的独立 Zephyr RTOS 版本。

本仓库的目标不是在旧工程里局部替换 RTOS，而是把板级定义、驱动层、服务层、通信链路、ROS 2 主机侧工具链和构建流程统一收口成一套可维护的 Zephyr 工程。

## 项目概览

| 项目 | 当前状态 |
|------|----------|
| 目标 MCU | `STM32F427IIH6` |
| 目标板卡 | `RSCF A` |
| Zephyr 基线 | `v4.3.0` |
| 默认工具链 | `gnuarmemb` |
| 默认下位机通信主链 | `USB CDC ACM + RSCF Link vNext` |
| 保留的兼容能力 | `mcu_comm` 代码生成与旧模块兼容层 |
| 板级硬件真源快照 | `docs/ioc_json/*.json` |

## 当前迁移状态

### 已完成并进入主线的部分

- 自定义开发板
  - `boards/arm/rscf_a_f427iih6/`
- Zephyr 应用入口与 profile 初始化链
  - `app/src/main.c`
  - `modules/rscf/core/rscf_app_profile.c`
- 基础 BSP 兼容层
  - `bsp_can`
  - `bsp_usart`
  - `bsp_log`
  - `bsp_dwt`
  - `bsp_spi`
  - `bsp_pwm`
  - `bsp_tim`
- 板级驱动与系统服务
  - `rscf_led_status`
  - `rscf_buzzer`
  - `rscf_power_switch`
  - `rscf_ext_uart_mux`
  - `rscf_event_bus`
  - `rscf_daemon_service`
  - `rscf_health_service`
  - `rscf_comm_service`
  - `rscf_chassis_service`
  - `rscf_motor_service`
  - `rscf_robot_service`
  - `rscf_ros_bridge`
  - `rscf_debug_fault`
- 设备链路
  - `Saber C3 IMU` 串口接入
  - `DTOF2510` 复用串口组接入
- 电机链路
  - `DJI Motor`
  - `DM Motor`
  - `Servo Motor` 的 PWM 舵机子集
- 数学库
  - 已切换为仓库内置 `CMSIS-DSP`
- 主机构件
  - `ros2_ws/` 中包含与下位机交互的 ROS 2 功能包与 launch 文件
  - `ros2_ws/src/rscf_link_bridge`
- CI
  - 当前已具备 `unit-tests`
  - 当前已具备 `firmware-build`
  - 当前已具备 `ros2-workspace-build`

### 本轮板级收敛已完成的内容

- 从旧工程导入完整 `.ioc` 解析 JSON 快照到 `docs/ioc_json/`
- 新增 `scripts/ioc/validate_board_from_ioc.py`
  - 构建前自动校验 Zephyr 板级 DTS 与导入的 `.ioc JSON` 是否一致
- 修正并补齐板级外设映射
  - `TIM5` 已回到旧工程一致的 `PH10/PH11/PH12/PI0`
  - `TIM8` PWM 资源已补入板级 DTS
  - `POWER1 ~ POWER4` 已进入 Zephyr 板级节点与驱动
  - 蜂鸣器已对齐到旧工程一致的 `TIM12_CH1 / PH6`
- 当前阶段按要求显式排除
  - `I2C2`
  - `OLED` 实机链路

### 仍未完成闭环的部分

下面这些模块不是“没有文件”，而是“还没有达到旧工程完整功能闭环”：

- APP 层完整角色
  - `arm`
  - `gimbal`
  - `BT`
  - 行为树 / 高层状态机
- 一部分兼容模块仍然只是骨架或半实装
  - `AD7190`
  - `HC05`
  - `mhall`
  - `remote_control`
  - `master_process / seasky_protocol`
  - `OLED`
- `servo motor`
  - 当前只支持 PWM 舵机
  - `bus servo` 尚未支持
- `robot.c / robot_task.h / robot_cmd.c`
  - 已可工作
  - 但尚未做到旧工程一比一的完整任务拓扑与命令分发闭环

## 默认通信方案

当前仓库默认主线是：

- 下位机端：`RSCF Link vNext runtime`
- 物理链路：`USB CDC ACM`
- 上位机端：`ros2_ws/src/rscf_link_bridge`
- 多 MCU 侧链路：`peer link (UART/CAN/SPI skeleton)`

仓库中仍保留了 `mcu_comm` 代码生成链和 `micro-ROS`，但它们当前都不是默认主线：

- `mcu_comm` 继续作为 compat/control-plane 生成基础保留
- `micro-ROS` 作为兼容后端保留，可按需重新启用

默认 ROS 2 主题约定：

- 下行：`/cmd_vel`
- 上行：`/rscf/system_status`
- 上行：`/odom`
- 上行：`/imu/data`

当前默认 bridge skeleton 已显式声明参数：

- `device_path`
- `baudrate`
- `compat_mode`

## 快速开始

### 1. 环境准备

建议环境：

- Linux
- `python3.12+`
- `cmake`
- 网络可访问，用于 `west update`
- 一套可用工具链
  - `Zephyr SDK`
  - 或 `STM32CubeCLT` 导出的 `gnuarmemb`

更完整的环境说明见：

- [docs/getting-started.md](docs/getting-started.md)

### 2. 初始化工作区

```bash
git clone git@github.com:Sonder4/rscf_a_zephyr.git
cd rscf_a_zephyr
./scripts/bootstrap/bootstrap.sh
source .venv/bin/activate
cd .workspace
```

### 3. 构建固件

```bash
west build -b rscf_a_f427iih6 ../app -d build_vnext
```

生成的主要产物位于：

- `.workspace/build_vnext/zephyr/zephyr.elf`
- `.workspace/build_vnext/zephyr/zephyr.hex`

### 4. 烧录固件

当前基线流程默认使用 J-Link：

```bash
west flash
```

如果你在本仓库里使用定制的烧录脚本或手工 J-Link 命令，请以当前开发板连接方式为准。

### 5. 主机侧 ROS 2 工作空间

```bash
source /opt/ros/humble/setup.bash
cd ros2_ws
colcon build
source install/setup.bash
ros2 launch rscf_link_bridge rscf_link_bridge.launch.py device_path:=/dev/ttyACM0 baudrate:=115200 compat_mode:=true
```

当前 `compat_mode` 默认值仍为 `true`，这是 bridge skeleton 阶段保留旧状态字和旧负载形态的兼容参数，不代表默认后端回退到 `micro-ROS`。

默认桥接 launch 当前会启动：

- `rscf_link_bridge_node`

旧的 `rscf_a_host` 与 `micro_ros_agent` 路径仍保留在仓库中，但属于兼容链路，不再是 README 默认路径。

## 板级与硬件来源

本仓库当前明确区分两类“硬件事实”：

- 旧工程和 CubeMX 导出的硬件配置事实
  - `docs/ioc_json/*.json`
- 当前 Zephyr 已经声明并编进主线的板级事实
  - `boards/arm/rscf_a_f427iih6/rscf_a_f427iih6.dts`
  - `boards/arm/rscf_a_f427iih6/rscf_a_f427iih6-pinctrl.dtsi`

当前构建流程已经把这两部分接起来：

- `app/CMakeLists.txt` 会调用 `scripts/ioc/validate_board_from_ioc.py`
- 如果关键 UART / CAN / SPI / TIM / USB 映射和导入的 `.ioc JSON` 不一致，构建会直接失败

## 仓库结构

- `app/`
  - Zephyr 应用入口、配置与 overlay
- `boards/arm/rscf_a_f427iih6/`
  - 自定义开发板定义
- `drivers/bsp/`
  - 旧 BSP 接口到 Zephyr 的兼容层
- `drivers/rscf/`
  - RSCF 板级设备驱动
- `modules/rscf/`
  - 服务层、设备层、应用 profile
- `generated/mcu_comm/`
  - compat endpoint / generated mirror
- `modules/compat/`
  - 旧工程兼容模块
- `modules/motor/`
  - 电机实现
- `third_party/cmsis_dsp/`
  - 内置 `CMSIS-DSP`
- `ros2_ws/`
  - 上位机 ROS 2 工作空间
- `docs/ioc_json/`
  - 旧工程 `.ioc` 解析后的 JSON 快照
- `.workspace/`
  - repo-local 的 west 工作空间，不是业务源码真源

## 文档入口

- 用户使用手册：[docs/zephyr-user-manual.md](docs/zephyr-user-manual.md)
- 上手说明：[docs/getting-started.md](docs/getting-started.md)
- 板级硬件映射：[docs/reference/rscf_a_hardware_map.md](docs/reference/rscf_a_hardware_map.md)
- `.ioc` 解析快照目录：[docs/ioc_json](docs/ioc_json)

## 已知限制

- 当前默认主线已经切换为 `USB CDC ACM + RSCF Link vNext + rscf_link_bridge`
- `micro-ROS` 为兼容后端，不再是默认启动链路
- `I2C2/OLED` 已从当前板级主线中暂时排除
- `TIM2` 在旧工程中属于混合用途
  - `PA0/PA1` 为编码器输入
  - `PA2/PA3` 为 `TIM2_CH3/CH4`
  - 当前 Zephyr 版本将其保留在 `bsp_tim` 兼容层中，不强行拆成多个互相冲突的 Zephyr 子节点
- 兼容层不等于完整驱动
  - 一部分模块已经能编译并接入系统
  - 但还需要真实硬件链路联调后，才能宣称完全替代旧工程实现

## 许可证说明

当前仓库已公开发布，但仓库内暂未附带单独的开源许可证文件。

如果你计划基于本仓库进行团队外分发、接受外部贡献或进行正式开源发布，建议尽快补充明确的 `LICENSE` 文件，而不是默认依赖“公开仓库”等同于“已授予开源使用权”。

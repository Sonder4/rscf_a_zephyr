# RSCF A Zephyr

`rscf_a_zephyr` 是 `RSCF_A` 原 FreeRTOS 工程面向 `STM32F427IIH6` 的独立 Zephyr RTOS 迁移仓库。

它的目标不是在旧仓库里局部替换 FreeRTOS，而是把板级定义、驱动、服务、通信链路、ROS2 主机侧工具和构建链全部收口成一套可独立维护的 Zephyr 版本。

## 1. 仓库定位

| 项目 | 当前配置 |
|------|----------|
| 目标 MCU | `STM32F427IIH6` |
| Zephyr 基线 | `v4.3.0` |
| 工作空间工具 | `west` |
| 默认工具链 | `gnuarmemb` |
| 默认通信主路径 | `USB CDC ACM + micro-ROS` |
| 主仓库 | `rscf_a_zephyr` |
| 旧工程参考源 | `/home/xuan/RC2026/STM32/RSCF_A` |

## 2. 当前已经完成的主线

截至 `2026-04-12`，当前仓库已经完成并验证到“可编译、可冷启动构建、可持续维护”的部分包括：

- 自定义开发板
  - `boards/arm/rscf_a_f427iih6/`
- Zephyr 应用入口与 profile 启动链
  - `app/src/main.c`
  - `modules/rscf/core/rscf_app_profile.c`
- Zephyr 化 BSP 适配
  - `bsp_can`
  - `bsp_usart`
  - `bsp_log`
  - `bsp_dwt`
  - `bsp_spi`
  - `bsp_pwm`
  - `bsp_tim`
- 板级驱动与服务
  - `rscf_led_status`
  - `rscf_buzzer`
  - `rscf_power_switch`
  - `rscf_ext_uart_mux`
  - `rscf_comm_service`
  - `rscf_chassis_service`
  - `rscf_motor_service`
  - `rscf_robot_service`
  - `rscf_event_bus`
  - `rscf_daemon_service`
  - `rscf_health_service`
  - `rscf_debug_fault`
  - `rscf_ros_bridge`
- 设备链路
  - `Saber C3 IMU` 串口接入
  - `DTOF2510` 复用串口组接入
- 电机链路
  - `DJI motor`
  - `DM motor`
  - `servo motor` 的 PWM 舵机子集
- 板级受控资源
  - `POWER1 ~ POWER4` 已进入 Zephyr 板级节点与驱动
  - 蜂鸣器已切回旧工程一致的 `TIM12/PH6`
  - `TIM5` 已切回旧工程一致的 `PH10/PH11/PH12/PI0`
  - `TIM8` PWM 资源已进入 Zephyr 板级 DTS
- 通信链路
  - `mcu_comm` 代码生成接入
  - `micro-ROS` MCU 端 USB CDC ACM 传输
  - `ros2_ws/` 主机侧 ROS2 工具链
- 数学库
  - 已从本地 `arm_math compat` 切换为 vendored `CMSIS-DSP`
- CI 基线
  - `unit-tests`
  - `ros2-workspace-build`
  - `firmware-build`
  - 当前已在干净 GitHub Actions 环境通过
- 硬件真源快照
  - `docs/ioc_json/` 已导入旧工程 `.ioc` 解析结果
  - 构建前会执行 `scripts/ioc/validate_board_from_ioc.py` 校验 Zephyr 板级配置与导入 JSON 是否一致

## 3. 旧工程里仍未完全迁移完成的内容

下面这部分不是“没有文件”，而是“还没有达到旧工程功能闭环”。

### 3.1 APP 层还缺完整业务实现

以下角色当前仍然是骨架或子集实现，不是完整应用：

- `arm`
  - 还没有形成真实机械臂控制任务、动作流程和执行器闭环
- `gimbal`
  - 目前只有状态位与 IMU/控制面对接，没有完整云台控制器
- `BT`
  - 目前只有 `HC05` 兼容链路，没有完整蓝牙业务状态机
- 行为树 / 高层状态机
  - 旧工程语义没有完整重建为 Zephyr 侧独立模块

### 3.2 `robot.c / robot_task.h / robot_cmd.c` 还没有做到旧工程全量对齐

当前 `modules/compat/robot.c`、`modules/compat/robot_cmd.c`、`include/robot_task.h` 已能工作，但仍有边界：

- `robot.c`
  - 已完成基础初始化编排
  - 但还不是旧工程整机启动拓扑的完整一比一复刻
- `robot_task.h`
  - 当前只保留核心周期定义
  - 没有完整恢复旧工程任务图
- `robot_cmd.c`
  - 已实现底盘控制面状态上报
  - 已实现 `ctrl_mode / heading_mode / velocity_frame` 三类 service
  - action 目前只有通用接收、启动、取消和阶段推进骨架
  - 还没有形成面向 `arm / gimbal / BT` 的真实命令分发闭环

### 3.3 一批外设目前还是“兼容层”而非“完整驱动”

以下模块当前已经进入主构建链，但从代码语义上看，还不是旧工程级别的完整硬件驱动：

- `AD7190`
  - 当前只检查 `spi4` 是否 ready，并维护缓存与 mock 电压
  - 尚未实现真实 SPI 寄存器读写、采样流程和数据解析
- `OLED`
  - 当前只维护内存中的字符串缓冲
  - 尚未实现真实 I2C 屏幕刷新
- `mhall`
  - 当前只有计数归一化与状态结构
  - 尚未绑定真实定时器/霍尔输入采集源
- `HC05`
  - 已有串口帧解析与在线状态维护
  - 但还缺独立 devicetree 建模与实机闭环验证
- `remote_control`
  - 已有 SBUS 解码与在线状态维护
  - 但仍需真实遥控链路长时间实机验证
- `master_process / seasky_protocol`
  - 协议打包解包与串口发送已有
  - 但还没有独立的 Zephyr native 服务封装和完整上下位机联调闭环

### 3.4 电机迁移仍有功能边界

- `servo motor`
  - 当前只支持 PWM 舵机
  - `bus servo` 在当前 Zephyr 版本中明确未支持

### 3.5 旧工程中被重构替代、但没有保留 1:1 旧接口的部分

这些内容并不是“完全没做”，而是已经按 Zephyr 思路重构，所以不再保留旧目录结构与旧接口语义：

- `USER/Modules/message_center`
  - 由 `modules/rscf/services/rscf_event_bus.c` 替代
- `USER/Modules/can_comm`
  - 由 `drivers/bsp/bsp_can.c` + `modules/rscf/services/rscf_comm_service.c` + 传输层替代
- `USER/Modules/ros/startTask.c`
  - 由 `modules/rscf/services/rscf_microros_service.c` 替代
- `USER/BSP/bsp_init`
  - 由 Zephyr 启动、board DTS、profile 初始化流程替代

如果目标是“旧 API 逐个保持兼容”，那么这些部分也还不能算完全迁移结束。

## 4. 结论

当前项目可以定义为：

- 已完成
  - Zephyr 主仓库、板级定义、驱动主线、micro-ROS 主链、CI 基线
- 未完成
  - 旧 APP 层完整角色闭环
  - 一批外设的真实驱动化与实机验收
  - 一部分旧 BSP 接口的一比一兼容

也就是说，当前仓库已经不是“最小 bring-up 工程”，但还不能宣称“旧 `RSCF_A` 全量功能已经全部迁移完成”。

## 5. 快速开始

### 5.1 固件构建

```bash
./scripts/bootstrap/bootstrap.sh
source .venv/bin/activate
cd .workspace
west build -b rscf_a_f427iih6 ../app -d build_microros
```

### 5.2 主机侧 ROS2 工作空间

```bash
source /opt/ros/humble/setup.bash
source /home/xuan/microros_ws/agent_ws/install/setup.bash
cd /home/xuan/RC2026/STM32/rscf_a_zephyr/ros2_ws
colcon build
source install/setup.bash
ros2 launch rscf_a_host rscf_host_io.launch.py start_agent:=true serial_device:=/dev/ttyACM0
```

### 5.3 默认通信主题

- 下行：`/cmd_vel`
- 下行：`/rscf/heading_target_deg`
- 上行：`/odom`
- 上行：`/imu/data`
- 上行：`/rscf/system_status`

## 6. 文档入口

- 用户使用手册：[docs/zephyr-user-manual.md](docs/zephyr-user-manual.md)
- 板级硬件映射：[docs/reference/rscf_a_hardware_map.md](docs/reference/rscf_a_hardware_map.md)
- `.ioc` 解析快照：[docs/ioc_json](docs/ioc_json)
- 上手说明：[docs/getting-started.md](docs/getting-started.md)

## 7. 仓库结构

- `app/`
  - Zephyr 应用入口与配置
- `boards/arm/rscf_a_f427iih6/`
  - 自定义开发板定义
- `drivers/bsp/`
  - 对旧 BSP 接口的 Zephyr 适配
- `docs/ioc_json/`
  - 从旧工程导入的 `.ioc` 解析 JSON 快照
- `drivers/rscf/`
  - 板级设备驱动
- `modules/rscf/`
  - 服务层、设备层、应用 profile
- `modules/compat/`
  - 旧工程兼容层
- `modules/motor/`
  - 电机实现
- `ros2_ws/`
  - 主机侧 ROS2 功能包与 launch

## 8. 说明

`.workspace/` 是 repo-local west 工作空间，不是业务源码真源。业务代码、板级代码、文档和主机构件的真源都在仓库根目录中维护。

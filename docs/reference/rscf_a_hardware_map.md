# RSCF A 硬件配置与 Zephyr 板级映射

## 1. 文档范围

本文档同时记录两类信息：

- 旧工程 `RSCF_A` 中已经确认的板卡物理资源
- 当前 `rscf_a_zephyr` 仓库里已经写入 DTS / overlay / 驱动代码的板级声明

这样做的目的是把“板子物理上有什么”和“Zephyr 现在实际声明了什么”分开写清楚，避免把待验证配置误当成已经完成的硬件事实。当前仓库还额外导入了旧工程 `ioc_json` 快照，并把其中关键外设清单接入了构建校验脚本。

本文档的依据主要来自：

- 旧工程 `README.md`
- 旧工程 `Docs/硬件配置说明.md`
- 旧工程 `Docs/ioc_json/*.json`
- 当前仓库 `docs/ioc_json/*.json`
- 当前 Zephyr 板级文件
  - `boards/arm/rscf_a_f427iih6/rscf_a_f427iih6.dts`
  - `boards/arm/rscf_a_f427iih6/rscf_a_f427iih6-pinctrl.dtsi`
  - `app/boards/rscf_a_f427iih6.overlay`
  - `app/boards/rscf_a_f427iih6_jlink_vcom.overlay`

## 2. 板卡基础信息

| 项目 | 旧工程硬件事实 | 当前 Zephyr 状态 |
|------|----------------|------------------|
| MCU | `STM32F427IIH6` | 已建模 |
| 核心 | `Cortex-M4` | 已建模 |
| HSE | `12 MHz` | 已建模 |
| SYSCLK | `168 MHz` | 已建模 |
| APB1 | `42 MHz` | 已建模 |
| APB2 | `84 MHz` | 已建模 |
| USB 时钟 | `48 MHz` | 已建模 |

当前 Zephyr 板级 DTS 中：

- `&clk_hse` 被设为 `12000000`
- `PLL` 设为 `M=6, N=168, P=2, Q=7`
- `rcc` 设为：
  - `clock-frequency = 168000000`
  - `ahb-prescaler = 1`
  - `apb1-prescaler = 4`
  - `apb2-prescaler = 2`

## 3. 当前 Zephyr 板级主节点

### 3.1 `chosen`

当前板级 `chosen` 设定为：

- `zephyr,console = &usart1`
- `zephyr,shell-uart = &usart1`
- `zephyr,sram = &sram0`
- `zephyr,flash = &flash0`

### 3.2 `aliases`

当前已经在 DTS 中建模的别名包括：

- `led0 = LED_G`
- `led1 = LED1`
- `led2 = LED2`
- `led3 = LED3`
- `led4 = LED4`
- `led5 = LED5`
- `led6 = LED6`
- `led7 = LED7`
- `led8 = LED8`
- `sw0 = 用户按键`

### 3.3 `rscf_board`

当前自定义板级聚合节点 `rscf_board` 已声明：

- `console-uart = usart1`
- `comm-uart = uart7`
- `imu-uart = usart6`
- `ext-mux-uart = uart8`
- `can-primary = can1`
- `can-secondary = can2`
- `usb-device = usbotg_fs`
- `status-led = LED_G`

这意味着当前 Zephyr 工程已经明确了一套“板级资源到业务语义”的映射。

## 4. 串口与 USB

| 资源 | 旧工程硬件事实 | 当前 Zephyr 声明 | 当前用途 / 状态 |
|------|----------------|------------------|-----------------|
| `USART1` | `PB6/PB7` | 已声明 | 当前默认 `console/shell` |
| `USART2` | `PD5/PD6` | 已声明 | 当前兼容层用于 `HC05` 默认句柄 |
| `USART3` | `PD8/PD9` | 已声明 | 当前兼容层用于 `Vision / master_process` 默认句柄 |
| `USART6` | `PG14/PG9` | 已声明 | 当前 `Saber C3 IMU` 主串口 |
| `UART7` | `PE8/PE7` | 已声明 | 当前默认 `comm-uart` |
| `UART8` | `PE1/PE0` | 已声明 | 当前 `ExtUartMux` 背板串口 |
| `USB OTG FS` | `PA11/PA12` | 已声明 | 通过 overlay 挂接 `CDC ACM` |

### 4.1 当前 USB 主链

当前默认 USB 通信路径为：

- `&usbotg_fs` 已启用
- `app/boards/rscf_a_f427iih6.overlay` 在其下挂载 `zephyr,cdc-acm-uart`
- `micro-ROS` 默认走 `USB CDC ACM`

### 4.2 J-Link VCOM 备选路径

`app/boards/rscf_a_f427iih6_jlink_vcom.overlay` 会把：

- `rscf_board.comm-uart`
  - 从默认的 `uart7`
  - 切换成 `usart1`

这属于调试/备选串口路由，不是默认 USB 主链。

## 5. CAN

| 资源 | 旧工程硬件事实 | 当前 Zephyr 声明 | 当前用途 / 状态 |
|------|----------------|------------------|-----------------|
| `CAN1` | `PD0/PD1`, `1 Mbps` | 已声明 | 当前主电机总线 |
| `CAN2` | `PB12/PB13`, `1 Mbps` | 已声明 | 当前副电机总线 |

当前 `drivers/bsp/bsp_can.c` 已通过 `rscf_board.can_primary` / `can_secondary` 解析设备，并在服务初始化时启动 `can1/can2`。

## 6. SPI 与 I2C

### 6.1 SPI

| 资源 | 旧工程硬件事实 | 当前 Zephyr 声明 | 当前用途 / 状态 |
|------|----------------|------------------|-----------------|
| `SPI4` | `PE4/PE5/PE6/PE12` | 已声明 | 当前兼容层中被 `AD7190` 视为默认总线 |
| `SPI5` | `PF6/PF7/PF8/PF9` | 已声明 | 当前未挂接具体设备节点 |

当前注意事项：

- `SPI4` 虽然已经在板级 DTS 中启用
- 但 `AD7190` 目前还只是兼容层实现
- 还没有写成“真实 SPI 从设备 + 寄存器访问 + 采样链”的完整 Zephyr 驱动

### 6.2 I2C

| 资源 | 旧工程硬件事实 | 当前 Zephyr 声明 | 当前用途 / 状态 |
|------|----------------|------------------|-----------------|
| `I2C2` | `PF0/PF1`, `400 kHz` | 当前显式禁用 | 按当前迁移范围，`I2C2/OLED` 暂不纳入 Zephyr 主线 |

当前处理原则已经固定为：

- 旧工程里 `PF0/PF1` 仍然保留为 `I2C2` 的历史事实
- 当前 Zephyr 版本为了保留 `ExtUartMux` 的 `PF0/PF1` 复用通道，把 `&i2c2` 显式设为 `disabled`
- 如果后续要恢复 `OLED/I2C2`，必须先重新分配 `PF0/PF1` 的硬件占用

## 7. PWM、蜂鸣器与舵机

### 7.1 旧工程物理资料

旧工程文档中给出的 PWM 资源包括：

- `TIM2`
- `TIM4`
- `TIM5`
- `TIM8`
- `TIM12`

其中旧工程把蜂鸣器标为：

- `TIM12_CH1 / PH6`

### 7.2 当前 Zephyr 板级声明

当前 DTS 中明确启用的 PWM 资源包括：

- `timers4 -> pwm4`
  - `PD12/PD13/PD14/PD15`
- `timers5 -> pwm5`
  - `PH10/PH11/PH12/PI0`
- `timers8 -> pwm8`
  - `PI5/PI6/PI7/PI2`
- `timers12 -> pwm12`
  - `PH6`

并且当前节点绑定为：

- `rscf_buzzer`
  - `pwms = <&pwm12 0 ...>`
- `rscf_servo_pwm`
  - `pwms = <&pwm5 0 ...>`

### 7.3 当前结论

这里需要明确写出：

- 旧工程文档把蜂鸣器描述为 `TIM12/PH6`
- 当前 Zephyr 板级已经改为 `timers12 -> pwm12 -> tim12_ch1_ph6`
- 旧工程 `TIM5` 四路 PWM 为 `PH10/PH11/PH12/PI0`
- 当前 Zephyr 板级已把之前错误的 `PA0~PA3` 映射纠正回旧工程一致的 `TIM5` 引脚
- 旧工程 `TIM8` 四路 PWM 为 `PI5/PI6/PI7/PI2`
- 当前 Zephyr 板级已补入 `timers8 -> pwm8`
- 旧工程 `TIM2` 是混合用途：
  - `PA0/PA1` 为编码器输入
  - `PA2/PA3` 为 `TIM2_CH3/CH4`
  - 当前 Zephyr 主线把 `TIM2` 保留为兼容层入口，由 `bsp_tim` 负责定时器/编码器寄存器级兼容，不把它拆成冲突的多个 Zephyr 子设备

所以当前状态应理解为：

- `rscf_buzzer` 已接入 Zephyr 主线
- 并且当前物理引脚/定时器声明已经对齐旧工程文档

同理：

- `servo motor` 当前只接入了 `pwm5` 单路 PWM 舵机方案
- `bus servo` 没有完成迁移

## 8. LED、按键与电源口

### 8.1 物理 LED

旧工程文档表明开发板上有：

- `LED_G`
- `LED1` 到 `LED8`

当前 Zephyr DTS 已全部声明：

- `LED_G = PF14`
- `LED1 = PG1`
- `LED2 = PG2`
- `LED3 = PG3`
- `LED4 = PG4`
- `LED5 = PG5`
- `LED6 = PG6`
- `LED7 = PG7`
- `LED8 = PG8`

这部分当前不是“聚合显示单灯”，而是已经按 9 个物理 LED 建模。

### 8.2 状态 LED 逻辑映射

当前 `drivers/rscf/rscf_led_status.c` 的状态通道映射为：

| 逻辑通道 | 物理 LED |
|----------|----------|
| `BOOT` | `LED1` |
| `MOTOR` | `LED2` |
| `DAEMON` | `LED3` |
| `ROBOT` | `LED4` |
| `CHASSIS` | `LED5` |
| `COMM` | `LED6` |
| `IMU` | `LED7` |
| `FAULT` | `LED8` |

这些 LED 的物理来源也已经和导入的 `docs/ioc_json/gpio_config.json` 对齐：

- `LED1 ~ LED8` 对应 `PG1 ~ PG8`
- `LED_G` 对应 `PF14`

当前通信链路状态灯使用：

- `LED6`
- 有效 `mcu_comm` 帧时快闪
- 等待连接 / 无数据时慢闪

### 8.3 用户按键

当前 DTS 中已声明：

- `PB2`
- 作为 `gpio-keys`
- 别名 `sw0`

### 8.4 电源口

旧工程文档中存在：

- `POWER1`
- `POWER2`
- `POWER3`
- `POWER4`

对应旧资料引脚为：

- `PH2`
- `PH3`
- `PH4`
- `PH5`

当前 Zephyr 板级 DTS 中已经新增：

- `rscf_power_switches`
- `power-gpios = <&gpioh 2>, <&gpioh 3>, <&gpioh 4>, <&gpioh 5>`

当前驱动中已经新增：

- `drivers/rscf/rscf_power_switch.c`

因此这部分当前状态是：

- 物理引脚已建模
- 驱动初始化已接入 profile
- 默认上电保持全部关闭

## 9. 当前已经进入 Zephyr 主线的板级设备

当前明确进入构建链并由 Zephyr 侧使用的板级资源有：

- `LED_G + LED1~LED8`
- `PB2` 用户按键
- `USART1/2/3/6`
- `UART7/8`
- `CAN1/2`
- `USB OTG FS`
- `SPI4/5`
- `I2C2`
- `ExtUartMux`
- `DTOF2510 group`
- `buzzer`
- `power switches`
- `servo pwm`

## 10. 当前仍未完成的板级接入项

从“旧工程硬件资源”到“Zephyr 已完成板级建模”的角度看，当前仍未完成的主要内容是：

- `AD7190` 还没有真实 SPI 从设备建模与驱动化
- `OLED` 还没有真实 I2C 从设备建模与驱动化
- `HC05`、`remote_control`、`vision/master_process` 还没有单独的 devicetree 子节点定义
- `I2C2` 与 `ExtUartMux(PF0/PF1)` 的资源冲突尚未完全收口

## 11. 建议的阅读顺序

如果你要继续补板级迁移，建议按下面顺序看：

1. 仓库总览：[README.md](/home/xuan/RC2026/STM32/rscf_a_zephyr/README.md)
2. 用户手册：[docs/zephyr-user-manual.md](/home/xuan/RC2026/STM32/rscf_a_zephyr/docs/zephyr-user-manual.md)
3. 当前板级 DTS：[rscf_a_f427iih6.dts](/home/xuan/RC2026/STM32/rscf_a_zephyr/boards/arm/rscf_a_f427iih6/rscf_a_f427iih6.dts)
4. 旧工程硬件说明：[硬件配置说明.md](/home/xuan/RC2026/STM32/RSCF_A/Docs/硬件配置说明.md)

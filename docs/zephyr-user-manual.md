# RSCF A Zephyr 用户使用手册

## 1. 文档目的

本文档面向当前 `rscf_a_zephyr` 仓库的使用者和后续维护者，回答四类问题：

1. 当前项目的 Zephyr 移植完成到什么程度了
2. 在本项目里“加载驱动”到底是什么意思
3. 如果我要继续新增一个驱动，应该改哪些文件
4. 如果我要继续写业务任务，应该沿用什么 Zephyr 结构

本文档基于当前仓库工作树编写，不是针对旧版 FreeRTOS 工程的通用说明。

---

## 2. 当前移植状态检查

### 2.1 已完成并可用于当前工程主线的部分

以下内容已经进入 Zephyr 主构建链，并且当前工程可编译通过：

- 自定义开发板定义
  - `boards/arm/rscf_a_f427iih6/`
- 自定义 devicetree binding
  - `dts/bindings/rscf/`
- Zephyr 应用入口与 profile 启动链
  - `app/src/main.c`
  - `modules/rscf/core/rscf_app_profile.c`
- 基础 BSP 适配
  - `bsp_can`
  - `bsp_usart`
  - `bsp_log`
  - `bsp_dwt`
- RSCF 驱动与服务
  - `rscf_led_status`
  - `rscf_buzzer`
  - `rscf_ext_uart_mux`
  - `rscf_dtof2510`
  - `rscf_imu_uart`
  - `rscf_comm_service`
  - `rscf_chassis_service`
  - `rscf_motor_service`
  - `rscf_robot_service`
  - `rscf_event_bus`
  - `rscf_daemon_service`
  - `rscf_health_service`
  - `rscf_ros_bridge`
- 电机相关模块
  - `DJI motor`
  - `DM motor`
  - `servo motor`
- 算法模块
  - `PID`
  - `Kalman_filter`
  - `coordinate_system`
  - `crc8/crc16`
  - `linear_algebra`
  - `user_lib`
  - `controller`
  - `unit_convert`
- 旧工程兼容模块骨架
  - `HC05`
  - `seasky_protocol`
  - `master_process`
  - `mhall`
  - `AD7190`
  - `OLED`
  - `remote_control`
  - `robot`
  - `robot_cmd`
- 通信链路
  - `USB CDC ACM`
  - `mcu_comm` 代码生成集成
  - `host ROS2 package` 可复用
- 数学库
  - 已从本地 `arm_math compat` 替换为 vendored `CMSIS-DSP`
  - 路径：`third_party/cmsis_dsp`

### 2.2 已接入但仍属于“硬件验证未完成”的部分

以下内容已经进入 Zephyr 工程，但还不能说“和原工程一样完成了外设级联调”：

- `AD7190`
- `HC05`
- `master_process / seasky_protocol`
- `mhall`
- `OLED`
- `remote_control`

这些模块现在的状态是：

- 已有头文件与源码入口
- 已进入 `app/CMakeLists.txt`
- 已能被业务层调用
- 但还缺少真实外设接线下的系统级验证

### 2.3 当前明确还是骨架的部分

以下部分目前是任务和状态机骨架，不是完整功能闭环：

- `arm`
- `gimbal`
- `BT`
- 一部分 `robot_cmd` action 逻辑

也就是说：

- 控制面回包、状态推进、任务拓扑已经有了
- 但还没有把完整机械臂/云台/行为树控制落进去

### 2.4 当前 ROS 方案的真实状态

当前项目的 ROS 主路径是：

- MCU 端：`RSCF Link vNext`
- 物理传输：`USB CDC ACM`
- Host 端：当前仓库内 `ros2_ws/src/rscf_link_bridge`
- 多 MCU 侧链路：`peer link (UART/CAN/SPI skeleton)`

当前 Host 端主线已经收口为：

- `rscf_link_bridge_node`
- `rscf_link_bridge.launch.py`

当前仓库仍保留：

- `micro-ROS`
- `rscf_a_host`

但它们当前都属于兼容路径，不再是默认 backend。

`modules/rscf/Kconfig` 当前默认 backend 是：

- `RSCF_ROS_BACKEND_BRIDGE`

当前 bridge skeleton 已声明的主题/参数包括：

- 下行：`/cmd_vel`
- 上行：`/rscf/system_status`
- 上行：`/odom`
- 上行：`/imu/data`
- 参数：`device_path`
- 参数：`baudrate`
- 参数：`compat_mode`

这里的 `compat_mode` 当前默认仍为 `true`，只是 bridge skeleton 阶段为了兼容旧状态字和旧负载形态而保留的参数，不代表默认 backend 已切回 `micro-ROS`。

### 2.5 当前工程如何判定“移植是活的”

至少满足以下四点时，可以认为当前 Zephyr 工程主线是活的：

- `west build -b rscf_a_f427iih6 ../app -d build_vnext` 成功
- 单元测试通过
- 板子可刷入 `zephyr.hex`
- 上电后 USB CDC 能稳定枚举，并能与 `rscf_link_bridge` 建链

---

## 3. 本项目里“加载驱动”是什么意思

### 3.1 不是 Linux 那种运行时加载

Zephyr 在这个项目里没有 `insmod`、`modprobe` 这类运行时驱动装载概念。

本项目里“加载驱动”实际指的是一套编译期接入流程：

1. 给硬件写 `binding`
2. 在板级 `dts` 或 overlay 里声明节点
3. 在 `Kconfig` / `prj.conf` 中使能功能
4. 在 `app/CMakeLists.txt` 把源码编进来
5. 在 `rscf_app_profile.c` 里初始化
6. 在 service 或 app 层调用

如果这六步里漏了任意一步，驱动一般都不会真正工作。

### 3.2 当前工程的驱动接入分层

当前工程大体分成四层：

#### 1. 板级与硬件描述层

- `boards/arm/rscf_a_f427iih6/rscf_a_f427iih6.dts`
- `app/boards/rscf_a_f427iih6.overlay`
- `dts/bindings/rscf/*.yaml`

这层负责回答：

- 设备挂在哪个外设上
- 用哪些引脚
- 默认波特率、PWM、GPIO 等参数是什么

#### 2. 驱动适配层

- `drivers/rscf/`
- `drivers/bsp/`
- `modules/rscf/devices/`

这层负责：

- 从 `devicetree` 取设备和参数
- 调 Zephyr 驱动 API
- 对外暴露更贴近 RSCF 的接口

例如：

- `drivers/rscf/rscf_ext_uart_mux.c`
- `drivers/rscf/rscf_buzzer.c`
- `modules/rscf/devices/rscf_imu_uart.c`

#### 3. 服务层

- `modules/rscf/services/`

这层负责：

- 驱动初始化顺序
- 周期调度
- 跨模块状态管理
- 事件总线、守护、控制面等系统级逻辑

例如：

- `rscf_motor_service.c`
- `rscf_chassis_service.c`
- `rscf_comm_service.c`
- `rscf_robot_service.c`

#### 4. 兼容/业务层

- `modules/compat/`
- `modules/motor/`
- `modules/algorithm/`

这层负责：

- 承接旧工程接口
- 放 APP 逻辑骨架
- 封装旧模块向 Zephyr 架构过渡

---

## 4. 当前驱动是如何被启用的

下面用现有工程的真实例子说明。

### 4.1 外部 UART 复用器 `ext_uart_mux`

它的接入路径是：

1. 定义 binding
   - `dts/bindings/rscf/ncurc,rscf-ext-uart-mux.yaml`
2. 在板级 DTS 里声明节点
   - `boards/arm/rscf_a_f427iih6/rscf_a_f427iih6.dts`
3. 在驱动里通过 `DT_NODELABEL(rscf_ext_uart_mux)` 取属性
   - `drivers/rscf/rscf_ext_uart_mux.c`
4. 在 profile 初始化时调用
   - `modules/rscf/core/rscf_app_profile.c`

驱动内部使用的是 Zephyr 原生方式：

- `GPIO_DT_SPEC_GET`
- `DEVICE_DT_GET`
- `gpio_pin_configure_dt`
- `gpio_pin_set_dt`

这就是推荐写法。

### 4.2 USB CDC ACM

它的接入路径是：

1. 板子启用 `usbotg_fs`
   - `boards/arm/rscf_a_f427iih6/rscf_a_f427iih6.dts`
2. 应用 overlay 给 USB 设备栈挂 `cdc_acm_uart0`
   - `app/boards/rscf_a_f427iih6.overlay`
3. `prj.conf` 里开启：
   - `CONFIG_USB_DEVICE_STACK=y`
   - `CONFIG_USB_CDC_ACM=y`
4. `mcu_comm` 的 `transport_usb.c` 使用该 CDC ACM 设备

### 4.3 IMU UART

当前是：

- 板级把 `imu-uart` 绑定到 `USART6`
- `rscf_imu_uart.c` 负责接收和解析
- `rscf_saber_imu_compat.c` 负责兼容旧 `IMU.h` 访问方式

### 4.4 电机控制

电机本体模块在：

- `modules/motor/`

而调度逻辑不在 `main()` 里，而是在：

- `modules/rscf/services/rscf_motor_service.c`

这里用 `k_work_delayable` 以 1 ms 周期调度 `MotorControlTask()`。

---

## 5. 如何新增一个驱动

建议先判断你要加的是哪一类驱动。

### 5.1 三种推荐放置方式

#### 方式 A：纯板级外设适配

适合：

- LED
- buzzer
- mux
- 某个简单 GPIO/PWM/I2C/SPI 外设

建议放在：

- `drivers/rscf/`

#### 方式 B：带协议解析的数据设备

适合：

- IMU
- 激光测距
- 某类 UART/SPI 传感器

建议放在：

- `modules/rscf/devices/`

#### 方式 C：旧工程接口兼容模块

适合：

- 需要先保持原接口名不变
- 暂时还没完全重构成 Zephyr-native

建议放在：

- `modules/compat/`

### 5.2 新增驱动的标准步骤

#### 步骤 1：先定义硬件节点

如果是自定义设备，先写 binding：

- `dts/bindings/rscf/ncurc,xxx.yaml`

再在板级 DTS 增加节点：

- `boards/arm/rscf_a_f427iih6/rscf_a_f427iih6.dts`

如果只是应用级追加，不想改主板 DTS，也可以放：

- `app/boards/rscf_a_f427iih6.overlay`

#### 步骤 2：写驱动源文件

优先使用 Zephyr API，不要新建一套 HAL 私有初始化流程。

推荐优先用：

- `DEVICE_DT_GET`
- `GPIO_DT_SPEC_GET`
- `PWM_DT_SPEC_GET`
- `i2c_*`
- `spi_*`
- `uart_*`

#### 步骤 3：暴露头文件

头文件放到：

- `drivers/rscf/`
- `include/`
- 或 `include/rscf/`

规则：

- 纯驱动头文件优先和源文件一起放在 `drivers/rscf/`
- 系统 service 接口放 `include/rscf/`
- 旧模块兼容接口放 `include/`

#### 步骤 4：加入构建

把 `.c` 文件加入：

- `app/CMakeLists.txt`

如果漏了这一步，文件存在也不会编进固件。

#### 步骤 5：加 Kconfig 开关

把功能开关加到：

- `modules/rscf/Kconfig`

再在 `prj.conf` 里决定是否强制开启。

当前已有的服务开关包括：

- `CONFIG_RSCF_EVENT_BUS`
- `CONFIG_RSCF_DAEMON_SERVICE`
- `CONFIG_RSCF_HEALTH_SERVICE`
- `CONFIG_RSCF_COMM_SERVICE`
- `CONFIG_RSCF_CHASSIS_SERVICE`
- `CONFIG_RSCF_ROBOT_SERVICE`
- `CONFIG_RSCF_MOTOR_SERVICE`
- `CONFIG_RSCF_DTOF2510`
- `CONFIG_RSCF_IMU_UART`

#### 步骤 6：接入 profile 初始化

驱动和服务的统一初始化入口在：

- `modules/rscf/core/rscf_app_profile.c`

你需要决定：

- 是在 `RSCFAppProfileInit()` 里初始化
- 还是仅在某个 service 初始化时初始化

#### 步骤 7：决定数据如何流动

推荐优先级：

1. 同类系统模块之间，用 `RSCFEventBus`
2. 强控制链和闭环状态，用专用 service 状态结构
3. 兼容旧模块时，保留旧 API 但内部转到 event bus 或 service

### 5.3 推荐示例模板

如果是简单周期设备，推荐沿用 service 模式：

```c
static struct k_work_delayable s_work;

static void MyWorkHandler(struct k_work *work)
{
    ARG_UNUSED(work);
    MyDriverPoll();
    (void)k_work_reschedule(&s_work, K_MSEC(10));
}

int MyServiceInit(void)
{
    k_work_init_delayable(&s_work, MyWorkHandler);
    (void)k_work_reschedule(&s_work, K_MSEC(10));
    return 0;
}
```

如果是低频轻量逻辑，也可以挂到 `RSCFAppProfileTick()`。

---

### 5.4 新增一个驱动的完整模板示例

下面给一个可以直接复制的“完整链路模板”。

这个模板不是伪代码，而是按当前 `rscf_a_zephyr` 仓库的真实结构组织的，目标是让你新增一个名为：

- `rscf_template_sensor`

的示例设备。

这个示例故意选成“简单 GPIO 采样设备”，因为它最容易看清整个接入链：

- `binding`
- `DTS 节点`
- `driver`
- `service`
- `compat`
- `Kconfig`
- `prj.conf`
- `app/CMakeLists.txt`
- `rscf_app_profile.c`

你后续如果要改成：

- UART 设备
- I2C 传感器
- SPI 设备
- CAN 外设

本质上只需要替换驱动内部的总线读写逻辑，整体接入骨架不变。

### 5.4.1 这个模板最终会形成什么结构

建议新增或修改这些文件：

- `dts/bindings/rscf/ncurc,rscf-template-sensor.yaml`
- `boards/arm/rscf_a_f427iih6/rscf_a_f427iih6.dts` 或 `app/boards/rscf_a_f427iih6.overlay`
- `drivers/rscf/rscf_template_sensor.h`
- `drivers/rscf/rscf_template_sensor.c`
- `include/rscf/rscf_template_sensor_service.h`
- `modules/rscf/services/rscf_template_sensor_service.c`
- `include/template_sensor.h`
- `modules/compat/template_sensor.c`
- `modules/rscf/Kconfig`
- `app/prj.conf`
- `app/CMakeLists.txt`
- `modules/rscf/core/rscf_app_profile.c`

其中职责建议是：

- `drivers/rscf/`：只管设备初始化和单次读写
- `modules/rscf/services/`：只管周期调度和数据发布
- `modules/compat/`：只管兼容旧接口

### 5.4.2 binding 模板

文件：

- `dts/bindings/rscf/ncurc,rscf-template-sensor.yaml`

可直接复制：

```yaml
description: RSCF template sensor driven by GPIOs

compatible: "ncurc,rscf-template-sensor"

include: base.yaml

properties:
  enable-gpios:
    type: phandle-array
    required: true

  sample-gpios:
    type: phandle-array
    required: true

  sample-period-ms:
    type: int
    default: 10
```

这个 binding 表示：

- 设备有一个使能 GPIO
- 设备有一个采样 GPIO
- 设备有一个默认采样周期

如果你是 UART/I2C/SPI 设备，就把这里的 `sample-gpios` 改成：

- `uart`
- `i2c-bus`
- `spi-max-frequency`
- `cs-gpios`

这一类属性。

### 5.4.3 DTS 节点模板

建议先放到：

- `app/boards/rscf_a_f427iih6.overlay`

这样不会改主板 `dts` 主文件。

可直接复制：

```dts
/ {
    rscf_template_sensor: template-sensor {
        compatible = "ncurc,rscf-template-sensor";
        enable-gpios = <&gpiod 12 GPIO_ACTIVE_HIGH>;
        sample-gpios = <&gpioe 3 GPIO_ACTIVE_HIGH>;
        sample-period-ms = <10>;
        status = "okay";
    };
};
```

这里需要你替换的只有三类信息：

- `rscf_template_sensor`
  - 节点 label，后续 `DT_NODELABEL()` 会用到
- `&gpiod 12`
  - 真实使能引脚
- `&gpioe 3`
  - 真实采样引脚

### 5.4.4 驱动头文件模板

文件：

- `drivers/rscf/rscf_template_sensor.h`

可直接复制：

```c
#ifndef RSCF_TEMPLATE_SENSOR_H_
#define RSCF_TEMPLATE_SENSOR_H_

#include <stdbool.h>
#include <stdint.h>

struct rscf_template_sensor_sample {
  uint32_t timestamp_ms;
  uint8_t level;
};

int RSCFTemplateSensorInit(void);
bool RSCFTemplateSensorReady(void);
uint32_t RSCFTemplateSensorSamplePeriodMs(void);
int RSCFTemplateSensorRead(struct rscf_template_sensor_sample *sample);

#endif /* RSCF_TEMPLATE_SENSOR_H_ */
```

这里建议注意两点：

- 设备驱动接口尽量保持“小而稳”
- 不要把周期调度、消息发布、状态机塞进 driver

### 5.4.5 驱动源文件模板

文件：

- `drivers/rscf/rscf_template_sensor.c`

可直接复制：

```c
#include "rscf_template_sensor.h"

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rscf_template_sensor, LOG_LEVEL_INF);

#define RSCF_TEMPLATE_SENSOR_NODE DT_NODELABEL(rscf_template_sensor)

#if DT_NODE_HAS_STATUS(RSCF_TEMPLATE_SENSOR_NODE, okay)
static const struct gpio_dt_spec s_enable =
  GPIO_DT_SPEC_GET(RSCF_TEMPLATE_SENSOR_NODE, enable_gpios);
static const struct gpio_dt_spec s_sample =
  GPIO_DT_SPEC_GET(RSCF_TEMPLATE_SENSOR_NODE, sample_gpios);
static const uint32_t s_sample_period_ms =
  DT_PROP(RSCF_TEMPLATE_SENSOR_NODE, sample_period_ms);
static bool s_ready;
#endif

int RSCFTemplateSensorInit(void)
{
#if DT_NODE_HAS_STATUS(RSCF_TEMPLATE_SENSOR_NODE, okay)
  int ret;

  if (!gpio_is_ready_dt(&s_enable) || !gpio_is_ready_dt(&s_sample)) {
    LOG_WRN("template sensor gpio not ready");
    s_ready = false;
    return -ENODEV;
  }

  ret = gpio_pin_configure_dt(&s_enable, GPIO_OUTPUT_INACTIVE);
  if (ret != 0) {
    return ret;
  }

  ret = gpio_pin_configure_dt(&s_sample, GPIO_INPUT);
  if (ret != 0) {
    return ret;
  }

  ret = gpio_pin_set_dt(&s_enable, 1);
  if (ret != 0) {
    return ret;
  }

  s_ready = true;
  LOG_INF("template sensor initialized");
  return 0;
#else
  return -ENODEV;
#endif
}

bool RSCFTemplateSensorReady(void)
{
#if DT_NODE_HAS_STATUS(RSCF_TEMPLATE_SENSOR_NODE, okay)
  return s_ready;
#else
  return false;
#endif
}

uint32_t RSCFTemplateSensorSamplePeriodMs(void)
{
#if DT_NODE_HAS_STATUS(RSCF_TEMPLATE_SENSOR_NODE, okay)
  return s_sample_period_ms;
#else
  return 10U;
#endif
}

int RSCFTemplateSensorRead(struct rscf_template_sensor_sample *sample)
{
#if DT_NODE_HAS_STATUS(RSCF_TEMPLATE_SENSOR_NODE, okay)
  int value;

  if ((sample == NULL) || (!s_ready)) {
    return -EINVAL;
  }

  value = gpio_pin_get_dt(&s_sample);
  if (value < 0) {
    return value;
  }

  sample->timestamp_ms = k_uptime_get_32();
  sample->level = (uint8_t)value;
  return 0;
#else
  ARG_UNUSED(sample);
  return -ENODEV;
#endif
}
```

你后续如果要迁移真实设备，通常只替换这几个点：

- `gpio_pin_get_dt()` 改成 `uart/i2c/spi/can` 的收发读取
- `struct rscf_template_sensor_sample` 改成真实数据结构
- `timestamp_ms` 改成你需要的时间戳形式

### 5.4.6 service 头文件模板

文件：

- `include/rscf/rscf_template_sensor_service.h`

可直接复制：

```c
#ifndef RSCF_TEMPLATE_SENSOR_SERVICE_H_
#define RSCF_TEMPLATE_SENSOR_SERVICE_H_

#include <stdbool.h>

int RSCFTemplateSensorServiceInit(void);
bool RSCFTemplateSensorServiceReady(void);

#endif /* RSCF_TEMPLATE_SENSOR_SERVICE_H_ */
```

### 5.4.7 service 源文件模板

文件：

- `modules/rscf/services/rscf_template_sensor_service.c`

可直接复制：

```c
#include <errno.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <rscf/rscf_event_bus.h>
#include <rscf/rscf_template_sensor_service.h>

#include "rscf_template_sensor.h"

LOG_MODULE_REGISTER(rscf_template_sensor_service, LOG_LEVEL_INF);

#define RSCF_TEMPLATE_SENSOR_TOPIC "template_sensor"

static struct k_work_delayable s_template_work;
static struct rscf_event_bus_publisher s_template_pub;
static bool s_template_service_ready;

static void RSCFTemplateSensorServiceWorkHandler(struct k_work *work)
{
  struct rscf_template_sensor_sample sample;

  ARG_UNUSED(work);

  if (RSCFTemplateSensorRead(&sample) == 0) {
    (void)RSCFEventBusPublish(&s_template_pub, &sample);
  }

  (void)k_work_reschedule(
    &s_template_work,
    K_MSEC(RSCFTemplateSensorSamplePeriodMs())
  );
}

int RSCFTemplateSensorServiceInit(void)
{
  int ret;

  if (s_template_service_ready) {
    return 0;
  }

  if (!RSCFTemplateSensorReady()) {
    return -ENODEV;
  }

  ret = RSCFEventBusPublisherRegister(
    RSCF_TEMPLATE_SENSOR_TOPIC,
    sizeof(struct rscf_template_sensor_sample),
    &s_template_pub
  );
  if (ret != 0) {
    return ret;
  }

  k_work_init_delayable(
    &s_template_work,
    RSCFTemplateSensorServiceWorkHandler
  );
  (void)k_work_reschedule(
    &s_template_work,
    K_MSEC(RSCFTemplateSensorSamplePeriodMs())
  );

  s_template_service_ready = true;
  LOG_INF("template sensor service initialized");
  return 0;
}

bool RSCFTemplateSensorServiceReady(void)
{
  return s_template_service_ready;
}
```

这个 service 做的事情很单纯：

- 周期读一次驱动
- 发布到 `RSCFEventBus`
- 不承担复杂业务状态机

如果你的设备不需要周期调度：

- 可以不写这个 service
- 直接让上层按需调用 `RSCFTemplateSensorRead()`

### 5.4.8 兼容层头文件模板

文件：

- `include/template_sensor.h`

可直接复制：

```c
#ifndef TEMPLATE_SENSOR_H_
#define TEMPLATE_SENSOR_H_

#include <stdbool.h>

#include "rscf_template_sensor.h"

int TemplateSensorInit(void);
int TemplateSensorGet(struct rscf_template_sensor_sample *sample, bool *updated);

#endif /* TEMPLATE_SENSOR_H_ */
```

### 5.4.9 兼容层源文件模板

文件：

- `modules/compat/template_sensor.c`

可直接复制：

```c
#include "template_sensor.h"

#include <errno.h>

#include <rscf/rscf_event_bus.h>

static struct rscf_event_bus_subscriber s_template_sub;
static bool s_template_compat_ready;

int TemplateSensorInit(void)
{
  int ret;

  if (s_template_compat_ready) {
    return 0;
  }

  ret = RSCFEventBusSubscriberRegister(
    "template_sensor",
    sizeof(struct rscf_template_sensor_sample),
    &s_template_sub
  );
  if (ret != 0) {
    return ret;
  }

  s_template_compat_ready = true;
  return 0;
}

int TemplateSensorGet(struct rscf_template_sensor_sample *sample, bool *updated)
{
  if ((sample == NULL) || (updated == NULL) || (!s_template_compat_ready)) {
    return -EINVAL;
  }

  return RSCFEventBusConsume(&s_template_sub, sample, updated);
}
```

这个 compat 层的目的不是再造一层复杂抽象，而是：

- 让旧 APP 代码还能继续用旧函数名
- 内部已经切到 Zephyr 的 service + event bus

### 5.4.10 Kconfig 模板

文件：

- `modules/rscf/Kconfig`

把下面内容加到同一个 `menu "RSCF Services"` 里：

```kconfig
config RSCF_TEMPLATE_SENSOR
  bool "Enable RSCF template sensor driver"
  default y

config RSCF_TEMPLATE_SENSOR_SERVICE
  bool "Enable RSCF template sensor service"
  depends on RSCF_TEMPLATE_SENSOR
  default y
```

### 5.4.11 prj.conf 模板

文件：

- `app/prj.conf`

可直接加：

```conf
CONFIG_RSCF_TEMPLATE_SENSOR=y
CONFIG_RSCF_TEMPLATE_SENSOR_SERVICE=y
```

如果你的 service 用到了事件总线，还要确认：

```conf
CONFIG_RSCF_EVENT_BUS=y
```

### 5.4.12 app/CMakeLists.txt 模板

文件：

- `app/CMakeLists.txt`

当前仓库已经把下面这些目录加进 include path：

- `../include`
- `../drivers/rscf`
- `../modules/rscf/services`

所以新增这个模板时，一般不需要再加 `include_directories`，只需要把源文件放进 `target_sources(app PRIVATE ...)`：

```cmake
  ../drivers/rscf/rscf_template_sensor.c
  ../modules/rscf/services/rscf_template_sensor_service.c
  ../modules/compat/template_sensor.c
```

如果你当前阶段不需要兼容层，就不要把：

- `../modules/compat/template_sensor.c`

编进去。

### 5.4.13 profile 初始化模板

文件：

- `modules/rscf/core/rscf_app_profile.c`

在 `RSCFAppProfileInit()` 里按顺序加：

```c
#if defined(CONFIG_RSCF_TEMPLATE_SENSOR)
  ret = RSCFTemplateSensorInit();
  if (ret != 0) {
    return ret;
  }
#endif

#if defined(CONFIG_RSCF_TEMPLATE_SENSOR_SERVICE)
  ret = RSCFTemplateSensorServiceInit();
  if (ret != 0) {
    return ret;
  }
#endif
```

然后在文件头部补 include：

```c
#include "rscf_template_sensor.h"
#include <rscf/rscf_template_sensor_service.h>
```

如果 compat 层只做订阅，不需要在 `profile` 初始化它，就让旧 `robot.c` 或对应业务初始化入口自己调：

- `TemplateSensorInit()`

### 5.4.14 如果你要改成 UART/I2C/SPI 驱动，应该替换哪里

直接对照下面替换就够了：

1. `binding`
   - 把 GPIO 属性替换成真实总线属性
2. `DTS`
   - 把节点挂到真实总线和引脚
3. `driver`
   - 把 `gpio_pin_get_dt()` 换成 `uart/i2c/spi` 收发逻辑
4. `sample struct`
   - 改成真实传感器输出格式
5. `service`
   - 维持周期读取 + 发布，不要顺手塞业务逻辑
6. `compat`
   - 只保留旧接口名，不要重复维护第二份状态机

### 5.4.15 最小验证清单

把模板复制进去以后，至少做下面五步：

1. `pytest tests/unit -q`
2. `cmake --build .workspace/build -j16`
3. 烧录开发板
4. 看 USB CDC 或 RTT 日志里是否出现初始化日志
5. 看上层 service / topic / compat 接口是否能拿到新数据

### 5.4.16 最后给一个“替换占位符清单”

你复制这个模板后，通常只需要系统性替换这些名字：

- `template_sensor`
  - 改成你的模块文件名
- `RSCFTemplateSensor`
  - 改成你的驱动接口前缀
- `template_sensor`
  - 改成你的 event bus topic 名
- `ncurc,rscf-template-sensor`
  - 改成你的 `compatible`
- `rscf_template_sensor`
  - 改成你的 `DT_NODELABEL`

建议替换顺序是：

1. 先改文件名
2. 再改 `compatible`
3. 再改 `DT_NODELABEL`
4. 最后改 topic 名和 sample 结构

按这个顺序改，最不容易漏。

---

## 6. 如何编写任务逻辑

### 6.1 不建议机械照搬 FreeRTOS 任务

旧工程里大量逻辑是按 `task + osDelay/vTaskDelayUntil` 写的。

在当前 Zephyr 工程里，更推荐按下面三种模式选一种：

### 6.2 模式一：`k_work_delayable`

适合：

- 高频
- 独立周期
- 不需要自己维持阻塞循环

当前工程里的代表：

- `modules/rscf/services/rscf_motor_service.c`

适合的任务：

- 电机控制
- 编码器采样
- 周期下发

### 6.3 模式二：profile 主循环 tick

适合：

- 低频
- 轻量
- 和系统主状态同步推进

当前入口：

- `app/src/main.c`
- `modules/rscf/core/rscf_app_profile.c`

`main()` 的主循环是固定 10 ms：

```c
while (1) {
  RSCFAppProfileTick();
  k_sleep(K_MSEC(10));
}
```

适合的逻辑：

- 简单健康监控
- 状态灯
- 非实时状态机推进

### 6.4 模式三：服务拥有状态机

适合：

- 控制面
- 行为状态机
- 多模块协调

当前代表：

- `modules/compat/robot_cmd.c`

这里的思路不是“起一个线程啥都干”，而是：

- service 拥有自己的运行时状态
- 定期调用一个 `Task()` 或 `Tick()`
- 所有动作状态都落在统一结构里

### 6.5 当前项目推荐的任务组织方式

建议优先按下面思路写：

1. 先写驱动，不写业务
2. 再写 service，管理驱动和调度
3. 最后写 app/compat 层状态机

不要一开始就把业务逻辑直接塞进：

- `main()`
- 中断回调
- 某个单独 `.c` 文件的无限循环

### 6.6 当前项目里的 APP 逻辑入口

现在 APP 层主入口是：

- `modules/compat/robot.c`

这里负责：

- 调 `RobotCmdInit()`
- 初始化 `Saber / HC05 / Vision / Remote / AD7190 / OLED / ROS bridge`
- 周期调用：
  - `RobotCmdTask()`
  - `VisionSend()`
  - `AD7190Task()`
  - `MHallTask()`
  - `RSCFRosBridgeSpinOnce()`
  - `OLEDTask()`

也就是说，当前项目已经把旧 `robot.c` 的职责改造成了“集中编排入口”。

---

## 7. 推荐的数据流写法

### 7.1 事件总线优先

当前事件总线在：

- `include/rscf/rscf_event_bus.h`
- `modules/rscf/services/rscf_event_bus.c`

推荐用法：

#### 发布端

```c
static struct rscf_event_bus_publisher s_pub;

RSCFEventBusPublisherRegister("my_topic", sizeof(my_data_t), &s_pub);
RSCFEventBusPublish(&s_pub, &data);
```

#### 订阅端

```c
static struct rscf_event_bus_subscriber s_sub;
bool updated = false;

RSCFEventBusSubscriberRegister("my_topic", sizeof(my_data_t), &s_sub);
RSCFEventBusConsume(&s_sub, &data, &updated);
```

### 7.2 什么时候不要用事件总线

以下情况可以不用：

- 单个 service 内部状态
- 高频控制环内部临时变量
- 明显只属于一个设备实例的数据

### 7.3 通信链路怎么接

当前 MCU 对外通信主路径是：

- `robot_interface.c`
- `comm_manager.c`
- `transport_usb.c`

业务层如果要发协议，不应该直接操作底层串口，而应该优先复用：

- `RobotSendChassisFeedback()`
- `RobotSendImuEuler()`
- `RobotSendSystemStatus()`

---

## 8. 如何继续迁移旧工程模块

建议按这个顺序。

### 8.1 第一步：先明确模块归属

把旧模块分成三类：

- 纯驱动型
- 设备协议型
- 业务状态机型

### 8.2 第二步：优先迁移“接口最稳定”的层

优先顺序建议：

1. 算法与工具模块
2. 设备驱动
3. service 层
4. APP 行为层

不要一开始就迁移复杂行为树或整机状态机。

### 8.3 第三步：保留兼容层，但不要永久依赖兼容层

`modules/compat/` 当前的定位是：

- 让旧接口先活下来
- 给后续重构留落脚点

长期目标仍然应该是：

- 把稳定模块逐步收敛到 `drivers/rscf/`、`modules/rscf/devices/`、`modules/rscf/services/`

---

## 9. 当前已知限制

### 9.1 并不是所有已迁移模块都完成了实机联调

当前“已迁移”不等于“已完成实机闭环验证”。

最容易误判的模块是：

- `AD7190`
- `HC05`
- `remote_control`
- `mhall`
- `OLED`

### 9.2 当前 ROS 主链路已经切到 bridge-default

当前默认链路是：

- MCU 侧运行 `RSCF Link vNext runtime`
- Host 侧运行 `ros2_ws/src/rscf_link_bridge`
- 物理主链路是 `USB CDC ACM`

仓库中仍然保留：

- `micro-ROS`
- `rscf_a_host`
- `mcu_comm` 代码生成链

但它们现在都属于兼容路径或历史资产，不再是当前仓库默认主线。

其中 `compat_mode` 默认仍为 `true`，只是 bridge skeleton 阶段用于保持旧状态字/旧负载形态的一层兼容参数，不应被理解成“系统默认走 micro-ROS”。

### 9.3 当前 APP 骨架仍偏底盘主线

`robot_cmd` 现在已经能处理：

- 控制模式切换
- 车头模式切换
- 速度参考系切换
- action 状态推进

但：

- 机械臂
- 云台
- 蓝牙行为层

仍是骨架，不是完整控制器。

---

## 10. 日常开发建议

### 10.1 改驱动前先问自己三个问题

1. 这个东西是 Zephyr 设备，还是旧接口兼容层
2. 它的配置应该放 DTS，还是放代码常量
3. 它应该由哪个 service 管理

### 10.2 推荐最小改动路径

如果只是继续扩一个驱动，建议只改这些地方：

- `dts/bindings/rscf/*.yaml`
- `boards/...dts` 或 `app/boards/...overlay`
- `drivers/rscf/` 或 `modules/rscf/devices/`
- `include/`
- `modules/rscf/Kconfig`
- `modules/rscf/core/rscf_app_profile.c`
- `app/CMakeLists.txt`

### 10.3 推荐验证顺序

每次加一个驱动后，最少做这些检查：

1. `pytest tests/unit -q`
2. `cmake --build .workspace/build -j16`
3. 烧录开发板
4. 看 USB 或 RTT 是否仍正常
5. 看该驱动对应 topic / 状态位 / 日志是否变化

---

## 11. 关键文件索引

### 启动与构建

- `app/CMakeLists.txt`
- `app/prj.conf`
- `app/src/main.c`

### 板级硬件描述

- `boards/arm/rscf_a_f427iih6/rscf_a_f427iih6.dts`
- `app/boards/rscf_a_f427iih6.overlay`
- `dts/bindings/rscf/`

### 服务总入口

- `modules/rscf/core/rscf_app_profile.c`

### 典型服务

- `modules/rscf/services/rscf_motor_service.c`
- `modules/rscf/services/rscf_chassis_service.c`
- `modules/rscf/services/rscf_robot_service.c`
- `modules/rscf/services/rscf_comm_service.c`
- `modules/rscf/services/rscf_event_bus.c`

### 典型驱动

- `drivers/rscf/rscf_ext_uart_mux.c`
- `drivers/rscf/rscf_buzzer.c`
- `modules/rscf/devices/rscf_imu_uart.c`

### APP / 兼容层

- `modules/compat/robot.c`
- `modules/compat/robot_cmd.c`

### 数学库

- `third_party/cmsis_dsp/`

---

## 12. 一句话结论

当前 `rscf_a_zephyr` 已经不是“最小 bring-up 工程”，而是一个可编译、可烧录、具备底盘主线与控制面骨架的 Zephyr 版本 RSCF A。

继续开发时，建议遵循这个顺序：

- 先用 `DTS + driver`
- 再接 `service`
- 最后落 `APP` 状态机

不要再把 FreeRTOS 时代的任务模型原样平移进来。

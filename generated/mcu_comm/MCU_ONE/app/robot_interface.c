/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  文 件 名: robot_interface.c
  版 本 号: 1.0.0
  作    者: Xuan
  生成日期: 2026-02-19
  功能描述: Zephyr 机器人应用接口实现
  补    充: 协议版本 2.0.0

*******************************************************************************/

#include "robot_interface.h"

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/util.h>

#include "bsp_dwt.h"
#include "bsp_log.h"

#define DEVICE_MID  0x05

static bool g_is_initialized = false;
static void* s_robot_tx_task_handle = NULL;
static uint32_t s_pending_tx_events;
static Transport_interface_t* s_transport_override = NULL;
static Transport_interface_t* s_transport_resolved = NULL;

__attribute__((weak)) void RobotInterfaceTxPeriodicHook(void)
{
}

__attribute__((weak)) void RobotInterfaceTxEventHook(uint32_t tx_events)
{
    ARG_UNUSED(tx_events);
}

__attribute__((weak)) Transport_interface_t* RobotInterfaceResolveTransportOverride(Transport_interface_t* transport)
{
    return transport;
}

__attribute__((weak)) void RobotInterfaceOnTransportReady(Transport_interface_t* transport)
{
    ARG_UNUSED(transport);
}

__attribute__((weak)) void RobotInterfaceFillCompatSystemStatus(uint8_t *control_plane_bytes, uint16_t len)
{
    ARG_UNUSED(control_plane_bytes);
    ARG_UNUSED(len);
}

__attribute__((weak)) void RobotInterfaceOnCompatSystemCmd(const uint8_t *control_plane_bytes, uint16_t len)
{
    ARG_UNUSED(control_plane_bytes);
    ARG_UNUSED(len);
}

static void RobotInterfaceCompatSystemCmdCallback(void* data)
{
    systemCmd_t *cmd = (systemCmd_t*)data;

    if (cmd != NULL)
    {
        RobotInterfaceOnCompatSystemCmd(cmd->cmd_bytes, sizeof(cmd->cmd_bytes));
    }

    SystemCmdCallback(data);
}


#if MCU_COMM_TEST_MODE
static uint32_t RobotInterfaceGetTickMs(void)
{
    return (uint32_t)DWT_GetTimeline_ms();
}
#endif

bool RobotInterfaceInit(void)
{
    int ret;

    if (g_is_initialized)
    {
        return true;
    }

    PID_Registry_Init();
    s_transport_resolved = RobotInterfaceResolveTransportOverride(s_transport_override);
    ret = Comm_Init(s_transport_resolved);
    if (ret != 0)
    {
        return false;
    }
    RobotInterfaceOnTransportReady(s_transport_resolved);

    PID_RegisterCallback(PID_CHASSIS_CTRL, ChassisCtrlCallback);
    PID_RegisterCallback(PID_MOTOR_DATA, MotorDataCallback);
    PID_RegisterCallback(PID_ROBOT_SWITCH, RobotSwitchCallback);
    PID_RegisterCallback(PID_SYSTEM_CMD, RobotInterfaceCompatSystemCmdCallback);

    g_is_initialized = true;
    return true;
}

void RobotInterfaceDeinit(void)
{
    g_is_initialized = false;
    s_pending_tx_events = 0U;
    s_transport_resolved = NULL;
}

void RobotInterfaceSetTransport(Transport_interface_t* transport)
{
    s_transport_override = transport;
}

void RobotInterfaceBindTxTask(void* task_handle)
{
    s_robot_tx_task_handle = task_handle;
}

void RobotInterfaceRequestTxEvent(uint32_t tx_events)
{
    ARG_UNUSED(s_robot_tx_task_handle);
    s_pending_tx_events |= tx_events;
}

bool RobotSendChassisFeedback(float vx_mps, float vy_mps, float odom_x_mm, float odom_y_mm)
{
    chassisFeedback_t data;

    if (!g_is_initialized)
    {
        return false;
    }

    data.vx_mps = vx_mps;
    data.vy_mps = vy_mps;
    data.odom_x_mm = odom_x_mm;
    data.odom_y_mm = odom_y_mm;
    Comm_Send(DEVICE_MID, PID_CHASSIS_FEEDBACK, (uint8_t*)&data, sizeof(data));
    return true;
}

bool RobotSendMotorData(float speed_rads, float position_rad, float torque_nm, uint8_t motor_num)
{
    motorData_t data;

    if (!g_is_initialized)
    {
        return false;
    }

    data.speed_rads = speed_rads;
    data.position_rad = position_rad;
    data.torque_nm = torque_nm;
    data.motor_num = motor_num;
    Comm_Send(DEVICE_MID, PID_MOTOR_DATA, (uint8_t*)&data, sizeof(data));
    return true;
}

bool RobotSendImuEuler(float roll, float pitch, float yaw)
{
    imuEuler_t data;

    if (!g_is_initialized)
    {
        return false;
    }

    data.roll = roll;
    data.pitch = pitch;
    data.yaw = yaw;
    Comm_Send(DEVICE_MID, PID_IMU_EULER, (uint8_t*)&data, sizeof(data));
    return true;
}

bool RobotSendImuQuaternion(float q0, float q1, float q2, float q3)
{
    imuQuaternion_t data;

    if (!g_is_initialized)
    {
        return false;
    }

    data.q0 = q0;
    data.q1 = q1;
    data.q2 = q2;
    data.q3 = q3;
    Comm_Send(DEVICE_MID, PID_IMU_QUATERNION, (uint8_t*)&data, sizeof(data));
    return true;
}

bool RobotSendSystemStatus(const uint8_t *control_plane_bytes)
{
    systemStatus_t data;

    if (!g_is_initialized)
    {
        return false;
    }

    memset(data.status_bytes, 0, sizeof(data.status_bytes));
    if (control_plane_bytes != NULL)
    {
        memcpy(data.status_bytes,
               control_plane_bytes,
               sizeof(data.status_bytes));
    }
    RobotInterfaceFillCompatSystemStatus(data.status_bytes,
                                         sizeof(data.status_bytes));

    Comm_Send(DEVICE_MID, PID_SYSTEM_STATUS, (uint8_t*)&data, sizeof(data));
    return true;
}


void RobotInterfaceProcessRx(void)
{
    if (!g_is_initialized)
    {
        return;
    }

    Comm_ProcessRx();
}

void RobotInterfaceProcessTxEvent(uint32_t tx_events)
{
    if (!g_is_initialized)
    {
        return;
    }

    RobotInterfaceTxEventHook(tx_events);
}

void RobotInterfaceProcessTxPeriodic(void)
{
    if (!g_is_initialized)
    {
        return;
    }

#if MCU_COMM_TEST_MODE
    RobotInterfaceTestTick();
#endif

    RobotInterfaceTxPeriodicHook();
}

void RobotInterfaceProcess(void)
{
    uint32_t tx_events;

    RobotInterfaceProcessRx();
    RobotInterfaceProcessTxPeriodic();

    tx_events = s_pending_tx_events;
    if (tx_events != 0U)
    {
        s_pending_tx_events = 0U;
        RobotInterfaceProcessTxEvent(tx_events);
    }
}

bool RobotIsConnected(void)
{
    return Comm_IsConnected();
}

bool RobotRxOverflowed(void)
{
    return Comm_HasRxOverflow();
}

void RobotInterfaceTestTick(void)
{
#if MCU_COMM_TEST_MODE
    static uint32_t s_last_tick;
    static uint16_t s_counter;
    uint32_t now = RobotInterfaceGetTickMs();

    if (!g_is_initialized)
    {
        return;
    }

    if ((uint32_t)(now - s_last_tick) < MCU_COMM_TEST_INTERVAL_MS)
    {
        return;
    }

    s_last_tick = now;
    s_counter++;

    (void)RobotSendChassisFeedback(0.5f + 0.01f * (float)(s_counter % 20U),
                                   -0.2f,
                                   (float)(s_counter * 10U),
                                   -(float)(s_counter * 5U));
    (void)RobotSendImuEuler(0.1f, 0.2f, 0.01f * (float)(s_counter % 314U));
    {
        uint8_t status_bytes[SYSTEM_STATUS_NUM_BYTES] = {0};
        status_bytes[0] = (uint8_t)(s_counter & 0x03U);
        (void)RobotSendSystemStatus(status_bytes);
    }
#endif
}

__attribute__((weak)) void ChassisCtrlCallback(void* data)
{
    ARG_UNUSED(data);
}

__attribute__((weak)) void MotorDataCallback(void* data)
{
    ARG_UNUSED(data);
}

__attribute__((weak)) void RobotSwitchCallback(void* data)
{
    ARG_UNUSED(data);
}

__attribute__((weak)) void SystemCmdCallback(void* data)
{
    ARG_UNUSED(data);
}


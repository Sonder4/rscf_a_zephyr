/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  文 件 名: data_def.c
  版 本 号: 1.0.0
  作    者: Xuan
  生成日期: 2026-02-11
  功能描述: Zephyr 通信协议数据结构序列化实现与PID注册表
  补    充: 协议版本 2.0.0

*******************************************************************************/

#include "data_def.h"

#include <string.h>

chassisCtrl_t g_chassis_ctrl;
chassisFeedback_t g_chassis_feedback;
motorData_t g_motor_data;
imuEuler_t g_imu_euler;
imuQuaternion_t g_imu_quaternion;
robotSwitch_t g_robot_switch;
systemStatus_t g_system_status;
systemCmd_t g_system_cmd;

PID_Entry_t pid_registry[256];

void PID_Registry_Init(void)
{
    memset(pid_registry, 0, sizeof(pid_registry));

    pid_registry[PID_CHASSIS_CTRL].target_mid = 0x05;
    pid_registry[PID_CHASSIS_CTRL].data_len = sizeof(chassisCtrl_t);
    pid_registry[PID_CHASSIS_CTRL].callback = NULL;
    pid_registry[PID_CHASSIS_CTRL].data_ptr = &g_chassis_ctrl;

    pid_registry[PID_CHASSIS_FEEDBACK].target_mid = 0x01;
    pid_registry[PID_CHASSIS_FEEDBACK].data_len = sizeof(chassisFeedback_t);
    pid_registry[PID_CHASSIS_FEEDBACK].callback = NULL;
    pid_registry[PID_CHASSIS_FEEDBACK].data_ptr = &g_chassis_feedback;

    pid_registry[PID_MOTOR_DATA].target_mid = 0x01;
    pid_registry[PID_MOTOR_DATA].data_len = sizeof(motorData_t);
    pid_registry[PID_MOTOR_DATA].callback = NULL;
    pid_registry[PID_MOTOR_DATA].data_ptr = &g_motor_data;

    pid_registry[PID_IMU_EULER].target_mid = 0x01;
    pid_registry[PID_IMU_EULER].data_len = sizeof(imuEuler_t);
    pid_registry[PID_IMU_EULER].callback = NULL;
    pid_registry[PID_IMU_EULER].data_ptr = &g_imu_euler;

    pid_registry[PID_IMU_QUATERNION].target_mid = 0x01;
    pid_registry[PID_IMU_QUATERNION].data_len = sizeof(imuQuaternion_t);
    pid_registry[PID_IMU_QUATERNION].callback = NULL;
    pid_registry[PID_IMU_QUATERNION].data_ptr = &g_imu_quaternion;

    pid_registry[PID_ROBOT_SWITCH].target_mid = 0x05;
    pid_registry[PID_ROBOT_SWITCH].data_len = sizeof(robotSwitch_t);
    pid_registry[PID_ROBOT_SWITCH].callback = NULL;
    pid_registry[PID_ROBOT_SWITCH].data_ptr = &g_robot_switch;

    pid_registry[PID_SYSTEM_STATUS].target_mid = 0x01;
    pid_registry[PID_SYSTEM_STATUS].data_len = sizeof(systemStatus_t);
    pid_registry[PID_SYSTEM_STATUS].callback = NULL;
    pid_registry[PID_SYSTEM_STATUS].data_ptr = &g_system_status;

    pid_registry[PID_SYSTEM_CMD].target_mid = 0x05;
    pid_registry[PID_SYSTEM_CMD].data_len = sizeof(systemCmd_t);
    pid_registry[PID_SYSTEM_CMD].callback = NULL;
    pid_registry[PID_SYSTEM_CMD].data_ptr = &g_system_cmd;

}

void PID_RegisterCallback(uint8_t pid, PID_Callback_t callback)
{
    pid_registry[pid].callback = callback;
}

uint16_t chassis_ctrl_serialize(const chassisCtrl_t* data, uint8_t* buf)
{
    memcpy(buf, data, sizeof(chassisCtrl_t));
    return sizeof(chassisCtrl_t);
}

uint16_t chassis_feedback_serialize(const chassisFeedback_t* data, uint8_t* buf)
{
    memcpy(buf, data, sizeof(chassisFeedback_t));
    return sizeof(chassisFeedback_t);
}

uint16_t motor_data_serialize(const motorData_t* data, uint8_t* buf)
{
    memcpy(buf, data, sizeof(motorData_t));
    return sizeof(motorData_t);
}

uint16_t imu_euler_serialize(const imuEuler_t* data, uint8_t* buf)
{
    memcpy(buf, data, sizeof(imuEuler_t));
    return sizeof(imuEuler_t);
}

uint16_t imu_quaternion_serialize(const imuQuaternion_t* data, uint8_t* buf)
{
    memcpy(buf, data, sizeof(imuQuaternion_t));
    return sizeof(imuQuaternion_t);
}

uint16_t robot_switch_serialize(const robotSwitch_t* data, uint8_t* buf)
{
    memcpy(buf, data, sizeof(robotSwitch_t));
    return sizeof(robotSwitch_t);
}

uint16_t system_status_serialize(const systemStatus_t* data, uint8_t* buf)
{
    memcpy(buf, data, sizeof(systemStatus_t));
    return sizeof(systemStatus_t);
}

uint16_t system_cmd_serialize(const systemCmd_t* data, uint8_t* buf)
{
    memcpy(buf, data, sizeof(systemCmd_t));
    return sizeof(systemCmd_t);
}


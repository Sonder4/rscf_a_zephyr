/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  文 件 名: robot_interface.h
  版 本 号: 1.0.0
  作    者: Xuan
  生成日期: 2026-02-19
  功能描述: Zephyr 机器人应用接口头文件
  补    充: 协议版本 2.0.0

*******************************************************************************/

#ifndef ROBOT_INTERFACE_H_
#define ROBOT_INTERFACE_H_

#include <stdbool.h>
#include <stdint.h>

#include "comm_manager.h"
#include "data_def.h"

#ifndef MCU_COMM_TEST_MODE
#define MCU_COMM_TEST_MODE 0
#endif

#ifndef MCU_COMM_TEST_INTERVAL_MS
#define MCU_COMM_TEST_INTERVAL_MS 100
#endif

#define ROBOT_INTERFACE_TX_EVENT_SYSTEM_STATUS    (1UL << 0)

bool RobotInterfaceInit(void);
void RobotInterfaceDeinit(void);
void RobotInterfaceSetTransport(Transport_interface_t* transport);
void RobotInterfaceProcess(void);
void RobotInterfaceProcessRx(void);
void RobotInterfaceProcessTxPeriodic(void);
void RobotInterfaceProcessTxEvent(uint32_t tx_events);
void RobotInterfaceBindTxTask(void* task_handle);
void RobotInterfaceRequestTxEvent(uint32_t tx_events);
bool RobotIsConnected(void);
bool RobotRxOverflowed(void);
void RobotInterfaceTestTick(void);
void RobotInterfaceTxPeriodicHook(void);
void RobotInterfaceTxEventHook(uint32_t tx_events);

bool RobotSendChassisFeedback(float vx_mps, float vy_mps, float odom_x_mm, float odom_y_mm);
bool RobotSendMotorData(float speed_rads, float position_rad, float torque_nm, uint8_t motor_num);
bool RobotSendImuEuler(float roll, float pitch, float yaw);
bool RobotSendImuQuaternion(float q0, float q1, float q2, float q3);
bool RobotSendSystemStatus(const uint8_t *control_plane_bytes);

void ChassisCtrlCallback(void* data);
void MotorDataCallback(void* data);
void RobotSwitchCallback(void* data);
void SystemCmdCallback(void* data);

#endif  /* ROBOT_INTERFACE_H_ */

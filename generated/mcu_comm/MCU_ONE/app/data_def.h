/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  文 件 名: data_def.h
  版 本 号: 1.0.0
  作    者: Xuan
  生成日期: 2026-02-11
  功能描述: Zephyr 通信协议数据结构定义与PID分配
  补    充: 此文件需与PC端完全一致，协议版本 2.0.0

*******************************************************************************/

#ifndef __DATA_DEF_H
#define __DATA_DEF_H

#include <stdint.h>
#include <string.h>

typedef enum
{
    PC_MASTER = 0x01,    /* 主控PC */
    MCU_CHASSIS = 0x03,    /* 底盘MCU */
    MCU_ARM = 0x04,    /* 机械臂MCU */
    MCU_ONE = 0x05    /* 一体化MCU（包含所有功能） */
} DeviceID_t;

typedef enum
{
    CONTROL_PLANE_IMU_STATE_DISABLED = 0,    /* IMU 状态 */
    CONTROL_PLANE_IMU_STATE_OK = 1,    /* IMU 状态 */
    CONTROL_PLANE_IMU_STATE_FAULT = 2    /* IMU 状态 */
} ControlPlaneImuState_e;

typedef enum
{
    CONTROL_PLANE_CHASSIS_CTRL_MODE_RELAX = 0,    /* 底盘控制模式 */
    CONTROL_PLANE_CHASSIS_CTRL_MODE_MANUAL = 1,    /* 底盘控制模式 */
    CONTROL_PLANE_CHASSIS_CTRL_MODE_CMD_VEL = 2,    /* 底盘控制模式 */
    CONTROL_PLANE_CHASSIS_CTRL_MODE_BRAKE_STOP = 3    /* 底盘控制模式 */
} ControlPlaneChassisCtrlMode_e;

typedef enum
{
    CONTROL_PLANE_HEADING_MODE_DISABLED = 0,    /* 车头控制模式 */
    CONTROL_PLANE_HEADING_MODE_LOCK_ZERO = 1,    /* 车头控制模式 */
    CONTROL_PLANE_HEADING_MODE_TARGET_HEADING = 2,    /* 车头控制模式 */
    CONTROL_PLANE_HEADING_MODE_FOLLOW_TARGET = 3    /* 车头控制模式 */
} ControlPlaneHeadingMode_e;

typedef enum
{
    CONTROL_PLANE_VELOCITY_FRAME_WORLD = 0,    /* 速度参考系 */
    CONTROL_PLANE_VELOCITY_FRAME_BODY = 1    /* 速度参考系 */
} ControlPlaneVelocityFrame_e;

typedef enum
{
    CONTROL_PLANE_SERVICE_ID_NONE = 0,    /* 轻量服务标识 */
    CONTROL_PLANE_SERVICE_ID_SET_CTRL_MODE = 1,    /* 轻量服务标识 */
    CONTROL_PLANE_SERVICE_ID_SET_HEADING_MODE = 2,    /* 轻量服务标识 */
    CONTROL_PLANE_SERVICE_ID_SET_VELOCITY_FRAME = 3    /* 轻量服务标识 */
} ControlPlaneServiceId_e;

typedef enum
{
    CONTROL_PLANE_SERVICE_RESULT_NONE = 0,    /* 轻量服务执行结果 */
    CONTROL_PLANE_SERVICE_RESULT_OK = 1,    /* 轻量服务执行结果 */
    CONTROL_PLANE_SERVICE_RESULT_REJECT = 2,    /* 轻量服务执行结果 */
    CONTROL_PLANE_SERVICE_RESULT_ERROR = 3    /* 轻量服务执行结果 */
} ControlPlaneServiceResult_e;

typedef enum
{
    CONTROL_PLANE_ACTION_ID_NONE = 0,    /* 轻量动作标识 */
    CONTROL_PLANE_ACTION_ID_STEP_TRANSITION = 1,    /* 轻量动作标识 */
    CONTROL_PLANE_ACTION_ID_ACQUIRE_KFS = 2,    /* 轻量动作标识 */
    CONTROL_PLANE_ACTION_ID_PLACE_KFS = 3    /* 轻量动作标识 */
} ControlPlaneActionId_e;

typedef enum
{
    CONTROL_PLANE_ACTION_CMD_START = 0,    /* 轻量动作控制命令 */
    CONTROL_PLANE_ACTION_CMD_CANCEL = 1,    /* 轻量动作控制命令 */
    CONTROL_PLANE_ACTION_CMD_QUERY = 2,    /* 轻量动作控制命令 */
    CONTROL_PLANE_ACTION_CMD_RESERVED = 3    /* 轻量动作控制命令 */
} ControlPlaneActionCmd_e;

typedef enum
{
    CONTROL_PLANE_ACTION_STATE_IDLE = 0,    /* 轻量动作执行状态 */
    CONTROL_PLANE_ACTION_STATE_ACCEPTED = 1,    /* 轻量动作执行状态 */
    CONTROL_PLANE_ACTION_STATE_RUNNING = 2,    /* 轻量动作执行状态 */
    CONTROL_PLANE_ACTION_STATE_CANCELING = 3,    /* 轻量动作执行状态 */
    CONTROL_PLANE_ACTION_STATE_SUCCEEDED = 4,    /* 轻量动作执行状态 */
    CONTROL_PLANE_ACTION_STATE_CANCELED = 5,    /* 轻量动作执行状态 */
    CONTROL_PLANE_ACTION_STATE_ABORTED = 6,    /* 轻量动作执行状态 */
    CONTROL_PLANE_ACTION_STATE_REJECTED = 7    /* 轻量动作执行状态 */
} ControlPlaneActionState_e;

typedef enum
{
    CONTROL_PLANE_ACTION_RESULT_NONE = 0,    /* 轻量动作最终结果 */
    CONTROL_PLANE_ACTION_RESULT_OK = 1,    /* 轻量动作最终结果 */
    CONTROL_PLANE_ACTION_RESULT_FAIL = 2,    /* 轻量动作最终结果 */
    CONTROL_PLANE_ACTION_RESULT_TIMEOUT = 3    /* 轻量动作最终结果 */
} ControlPlaneActionResult_e;

typedef enum
{
    CONTROL_PLANE_STEP_TRANSITION_GOAL_UP_STEP = 0,    /* 上下台阶目标 */
    CONTROL_PLANE_STEP_TRANSITION_GOAL_DOWN_STEP = 1    /* 上下台阶目标 */
} ControlPlaneStepTransitionGoal_e;

typedef enum
{
    CONTROL_PLANE_STEP_TRANSITION_TASK_STATE_IDLE = 0,    /* 上下台阶动作阶段 */
    CONTROL_PLANE_STEP_TRANSITION_TASK_STATE_UP_STEP = 1,    /* 上下台阶动作阶段 */
    CONTROL_PLANE_STEP_TRANSITION_TASK_STATE_DOWN_STEP = 2,    /* 上下台阶动作阶段 */
    CONTROL_PLANE_STEP_TRANSITION_TASK_STATE_UP_STEP_DONE = 3,    /* 上下台阶动作阶段 */
    CONTROL_PLANE_STEP_TRANSITION_TASK_STATE_DOWN_STEP_DONE = 4    /* 上下台阶动作阶段 */
} ControlPlaneStepTransitionTaskState_e;

typedef enum
{
    CONTROL_PLANE_ACQUIRE_KFS_GOAL_PLUS_20CM = 0,    /* 取 KFS 目标位置 */
    CONTROL_PLANE_ACQUIRE_KFS_GOAL_MINUS_20CM = 1,    /* 取 KFS 目标位置 */
    CONTROL_PLANE_ACQUIRE_KFS_GOAL_PLUS_40CM = 2    /* 取 KFS 目标位置 */
} ControlPlaneAcquireKfsGoal_e;

typedef enum
{
    CONTROL_PLANE_ACQUIRE_KFS_TASK_STATE_IDLE = 0,    /* 取 KFS 动作阶段 */
    CONTROL_PLANE_ACQUIRE_KFS_TASK_STATE_STARTED = 1,    /* 取 KFS 动作阶段 */
    CONTROL_PLANE_ACQUIRE_KFS_TASK_STATE_APPROACHING = 2,    /* 取 KFS 动作阶段 */
    CONTROL_PLANE_ACQUIRE_KFS_TASK_STATE_ACQUIRING = 3,    /* 取 KFS 动作阶段 */
    CONTROL_PLANE_ACQUIRE_KFS_TASK_STATE_ACQUIRE_SUCCESS = 4,    /* 取 KFS 动作阶段 */
    CONTROL_PLANE_ACQUIRE_KFS_TASK_STATE_ACQUIRE_FAIL = 5,    /* 取 KFS 动作阶段 */
    CONTROL_PLANE_ACQUIRE_KFS_TASK_STATE_PLACING = 6,    /* 取 KFS 动作阶段 */
    CONTROL_PLANE_ACQUIRE_KFS_TASK_STATE_PLACE_SUCCESS = 7,    /* 取 KFS 动作阶段 */
    CONTROL_PLANE_ACQUIRE_KFS_TASK_STATE_PLACE_FAIL = 8,    /* 取 KFS 动作阶段 */
    CONTROL_PLANE_ACQUIRE_KFS_TASK_STATE_TASK_DONE = 9,    /* 取 KFS 动作阶段 */
    CONTROL_PLANE_ACQUIRE_KFS_TASK_STATE_TASK_FAIL = 10    /* 取 KFS 动作阶段 */
} ControlPlaneAcquireKfsTaskState_e;

typedef enum
{
    CONTROL_PLANE_PLACE_KFS_GOAL_PICK_KFS = 0,    /* 放置 KFS 动作命令 */
    CONTROL_PLANE_PLACE_KFS_GOAL_OPEN_SUCTION = 1,    /* 放置 KFS 动作命令 */
    CONTROL_PLANE_PLACE_KFS_GOAL_CLOSE_SUCTION = 2,    /* 放置 KFS 动作命令 */
    CONTROL_PLANE_PLACE_KFS_GOAL_LIFT_KFS = 3    /* 放置 KFS 动作命令 */
} ControlPlanePlaceKfsGoal_e;

typedef enum
{
    CONTROL_PLANE_PLACE_KFS_TASK_STATE_IDLE = 0,    /* 放置 KFS 动作阶段 */
    CONTROL_PLANE_PLACE_KFS_TASK_STATE_PICKING_KFS = 1,    /* 放置 KFS 动作阶段 */
    CONTROL_PLANE_PLACE_KFS_TASK_STATE_ACQUIRE_SUCCESS = 2,    /* 放置 KFS 动作阶段 */
    CONTROL_PLANE_PLACE_KFS_TASK_STATE_ACQUIRE_FAIL = 3,    /* 放置 KFS 动作阶段 */
    CONTROL_PLANE_PLACE_KFS_TASK_STATE_PLACING = 4,    /* 放置 KFS 动作阶段 */
    CONTROL_PLANE_PLACE_KFS_TASK_STATE_PLACE_SUCCESS = 5,    /* 放置 KFS 动作阶段 */
    CONTROL_PLANE_PLACE_KFS_TASK_STATE_PLACE_FAIL = 6,    /* 放置 KFS 动作阶段 */
    CONTROL_PLANE_PLACE_KFS_TASK_STATE_OPEN_SUCTION = 7,    /* 放置 KFS 动作阶段 */
    CONTROL_PLANE_PLACE_KFS_TASK_STATE_CLOSE_SUCTION = 8    /* 放置 KFS 动作阶段 */
} ControlPlanePlaceKfsTaskState_e;


#define PID_CHASSIS_CTRL        0x01    /* 底盘控制指令 */
#define PID_CHASSIS_FEEDBACK        0x02    /* 底盘运动状态反馈 */
#define PID_MOTOR_DATA        0x03    /* 电机控制指令/反馈 */
#define PID_IMU_EULER        0x04    /* IMU欧拉角原始数据（deg） */
#define PID_IMU_QUATERNION        0x05    /* IMU四元数数据 */
#define PID_ROBOT_SWITCH        0x06    /* 机器人开关量 */
#define PID_SYSTEM_STATUS        0x07    /* 系统状态反馈/同步 */
#define PID_SYSTEM_CMD        0x09    /* 系统控制命令 */

#define SYSTEM_STATUS_NUM_BYTES              5U
#define SYSTEM_STATUS_INIT_DONE_OFFSET          0U
#define SYSTEM_STATUS_INIT_DONE_WIDTH           1U
#define SYSTEM_STATUS_INIT_DONE_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_INIT_DONE_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_ODOM_OK_OFFSET          1U
#define SYSTEM_STATUS_ODOM_OK_WIDTH           1U
#define SYSTEM_STATUS_ODOM_OK_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_ODOM_OK_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_TASK_ALL_OK_OFFSET          2U
#define SYSTEM_STATUS_TASK_ALL_OK_WIDTH           1U
#define SYSTEM_STATUS_TASK_ALL_OK_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_TASK_ALL_OK_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_PEER_MCU_COMM_OK_OFFSET          3U
#define SYSTEM_STATUS_PEER_MCU_COMM_OK_WIDTH           1U
#define SYSTEM_STATUS_PEER_MCU_COMM_OK_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_PEER_MCU_COMM_OK_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_IMU_STATE_OFFSET          4U
#define SYSTEM_STATUS_IMU_STATE_WIDTH           2U
#define SYSTEM_STATUS_IMU_STATE_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_IMU_STATE_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_CTRL_MODE_OFFSET          6U
#define SYSTEM_STATUS_CTRL_MODE_WIDTH           2U
#define SYSTEM_STATUS_CTRL_MODE_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_CTRL_MODE_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_HEADING_MODE_OFFSET          8U
#define SYSTEM_STATUS_HEADING_MODE_WIDTH           2U
#define SYSTEM_STATUS_HEADING_MODE_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_HEADING_MODE_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_VELOCITY_FRAME_OFFSET          10U
#define SYSTEM_STATUS_VELOCITY_FRAME_WIDTH           1U
#define SYSTEM_STATUS_VELOCITY_FRAME_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_VELOCITY_FRAME_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_SERVICE_ID_OFFSET          16U
#define SYSTEM_STATUS_SERVICE_ID_WIDTH           2U
#define SYSTEM_STATUS_SERVICE_ID_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_SERVICE_ID_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_SERVICE_RESULT_OFFSET          18U
#define SYSTEM_STATUS_SERVICE_RESULT_WIDTH           2U
#define SYSTEM_STATUS_SERVICE_RESULT_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_SERVICE_RESULT_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_SERVICE_TICKET_OFFSET          20U
#define SYSTEM_STATUS_SERVICE_TICKET_WIDTH           2U
#define SYSTEM_STATUS_SERVICE_TICKET_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_SERVICE_TICKET_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_ACTION_ID_OFFSET          24U
#define SYSTEM_STATUS_ACTION_ID_WIDTH           2U
#define SYSTEM_STATUS_ACTION_ID_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_ACTION_ID_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_ACTION_STATE_OFFSET          26U
#define SYSTEM_STATUS_ACTION_STATE_WIDTH           3U
#define SYSTEM_STATUS_ACTION_STATE_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_ACTION_STATE_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_ACTION_TICKET_OFFSET          29U
#define SYSTEM_STATUS_ACTION_TICKET_WIDTH           2U
#define SYSTEM_STATUS_ACTION_TICKET_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_ACTION_TICKET_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_ACTION_RESULT_OFFSET          32U
#define SYSTEM_STATUS_ACTION_RESULT_WIDTH           2U
#define SYSTEM_STATUS_ACTION_RESULT_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_ACTION_RESULT_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_STATUS_ACTION_PROGRESS_OFFSET          34U
#define SYSTEM_STATUS_ACTION_PROGRESS_WIDTH           6U
#define SYSTEM_STATUS_ACTION_PROGRESS_SYNC_DIRECTION  SYSTEM_STATUS_DIR_MCU_TO_PC
#define SYSTEM_STATUS_ACTION_PROGRESS_APPLY_MODE      SYSTEM_STATUS_APPLY_PASSIVE
#define SYSTEM_CMD_NUM_BYTES              3U
#define SYSTEM_CMD_SERVICE_ID_OFFSET          0U
#define SYSTEM_CMD_SERVICE_ID_WIDTH           2U
#define SYSTEM_CMD_SERVICE_ID_SYNC_DIRECTION  SYSTEM_CMD_DIR_PC_TO_MCU
#define SYSTEM_CMD_SERVICE_ID_APPLY_MODE      SYSTEM_CMD_APPLY_PASSIVE
#define SYSTEM_CMD_SERVICE_ARG_OFFSET          2U
#define SYSTEM_CMD_SERVICE_ARG_WIDTH           2U
#define SYSTEM_CMD_SERVICE_ARG_SYNC_DIRECTION  SYSTEM_CMD_DIR_PC_TO_MCU
#define SYSTEM_CMD_SERVICE_ARG_APPLY_MODE      SYSTEM_CMD_APPLY_PASSIVE
#define SYSTEM_CMD_SERVICE_TICKET_OFFSET          4U
#define SYSTEM_CMD_SERVICE_TICKET_WIDTH           2U
#define SYSTEM_CMD_SERVICE_TICKET_SYNC_DIRECTION  SYSTEM_CMD_DIR_PC_TO_MCU
#define SYSTEM_CMD_SERVICE_TICKET_APPLY_MODE      SYSTEM_CMD_APPLY_PASSIVE
#define SYSTEM_CMD_ACTION_ID_OFFSET          8U
#define SYSTEM_CMD_ACTION_ID_WIDTH           2U
#define SYSTEM_CMD_ACTION_ID_SYNC_DIRECTION  SYSTEM_CMD_DIR_PC_TO_MCU
#define SYSTEM_CMD_ACTION_ID_APPLY_MODE      SYSTEM_CMD_APPLY_PASSIVE
#define SYSTEM_CMD_ACTION_CMD_OFFSET          10U
#define SYSTEM_CMD_ACTION_CMD_WIDTH           2U
#define SYSTEM_CMD_ACTION_CMD_SYNC_DIRECTION  SYSTEM_CMD_DIR_PC_TO_MCU
#define SYSTEM_CMD_ACTION_CMD_APPLY_MODE      SYSTEM_CMD_APPLY_PASSIVE
#define SYSTEM_CMD_ACTION_TICKET_OFFSET          12U
#define SYSTEM_CMD_ACTION_TICKET_WIDTH           2U
#define SYSTEM_CMD_ACTION_TICKET_SYNC_DIRECTION  SYSTEM_CMD_DIR_PC_TO_MCU
#define SYSTEM_CMD_ACTION_TICKET_APPLY_MODE      SYSTEM_CMD_APPLY_PASSIVE
#define SYSTEM_CMD_ACTION_PARAM_OFFSET          16U
#define SYSTEM_CMD_ACTION_PARAM_WIDTH           8U
#define SYSTEM_CMD_ACTION_PARAM_SYNC_DIRECTION  SYSTEM_CMD_DIR_PC_TO_MCU
#define SYSTEM_CMD_ACTION_PARAM_APPLY_MODE      SYSTEM_CMD_APPLY_PASSIVE

#pragma pack(1)
typedef struct
{
    int16_t speed_mps;
    int16_t direction_cdeg;
    int16_t yaw_rate_radps;
    int16_t heading_cdeg;
} chassisCtrl_t;

typedef struct
{
    float vx_mps;
    float vy_mps;
    float odom_x_mm;
    float odom_y_mm;
} chassisFeedback_t;

typedef struct
{
    float speed_rads;
    float position_rad;
    float torque_nm;
    uint8_t motor_num;
} motorData_t;

typedef struct
{
    float roll;
    float pitch;
    float yaw;
} imuEuler_t;

typedef struct
{
    float q0;
    float q1;
    float q2;
    float q3;
} imuQuaternion_t;

typedef struct
{
    uint16_t switch_mask;
} robotSwitch_t;

typedef struct
{
    uint8_t status_bytes[SYSTEM_STATUS_NUM_BYTES];
} systemStatus_t;

typedef struct
{
    uint8_t cmd_bytes[SYSTEM_CMD_NUM_BYTES];
} systemCmd_t;

#pragma pack()

typedef void (*PID_Callback_t)(void* data);

typedef struct
{
    uint8_t target_mid;
    uint8_t data_len;
    PID_Callback_t callback;
    void* data_ptr;
} PID_Entry_t;

extern PID_Entry_t pid_registry[256];

extern chassisCtrl_t g_chassis_ctrl;
extern chassisFeedback_t g_chassis_feedback;
extern motorData_t g_motor_data;
extern imuEuler_t g_imu_euler;
extern imuQuaternion_t g_imu_quaternion;
extern robotSwitch_t g_robot_switch;
extern systemStatus_t g_system_status;
extern systemCmd_t g_system_cmd;

void PID_Registry_Init(void);
void PID_RegisterCallback(uint8_t pid, PID_Callback_t callback);

uint16_t chassis_ctrl_serialize(const chassisCtrl_t* data, uint8_t* buf);

uint16_t chassis_feedback_serialize(const chassisFeedback_t* data, uint8_t* buf);

uint16_t motor_data_serialize(const motorData_t* data, uint8_t* buf);

uint16_t imu_euler_serialize(const imuEuler_t* data, uint8_t* buf);

uint16_t imu_quaternion_serialize(const imuQuaternion_t* data, uint8_t* buf);

uint16_t robot_switch_serialize(const robotSwitch_t* data, uint8_t* buf);

uint16_t system_status_serialize(const systemStatus_t* data, uint8_t* buf);

uint16_t system_cmd_serialize(const systemCmd_t* data, uint8_t* buf);


#endif /* __DATA_DEF_H */

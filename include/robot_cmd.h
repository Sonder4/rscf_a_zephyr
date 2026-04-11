#ifndef ROBOT_CMD_H_
#define ROBOT_CMD_H_

#include <stdint.h>

#include "data_def.h"

typedef enum
{
    ROBOT_ARM_STATE_IDLE = 0,
    ROBOT_ARM_STATE_HOMING = 1,
    ROBOT_ARM_STATE_EXECUTING = 2,
    ROBOT_ARM_STATE_HOLDING = 3
} RobotArmState_e;

typedef enum
{
    ROBOT_GIMBAL_STATE_DISABLED = 0,
    ROBOT_GIMBAL_STATE_STABILIZE = 1,
    ROBOT_GIMBAL_STATE_TRACK = 2
} RobotGimbalState_e;

typedef enum
{
    ROBOT_BT_STATE_DISCONNECTED = 0,
    ROBOT_BT_STATE_LINKED = 1
} RobotBtState_e;

typedef enum
{
    ROBOT_COMM_STATE_IDLE = 0,
    ROBOT_COMM_STATE_USB_ACTIVE = 1
} RobotCommState_e;

void RobotCmdInit(void);
void RobotCmdTask(void);
void RobotCmdCommPublishTick(void);
void RobotCmdCommPublishNow(void);
void RobotCmdHandleSystemCmd(const systemCmd_t *cmd);
RobotArmState_e RobotCmdGetArmState(void);
RobotGimbalState_e RobotCmdGetGimbalState(void);
RobotBtState_e RobotCmdGetBtState(void);
RobotCommState_e RobotCmdGetCommState(void);

#endif /* ROBOT_CMD_H_ */

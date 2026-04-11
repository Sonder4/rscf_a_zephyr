#include <stdbool.h>
#include <string.h>

#include <zephyr/kernel.h>

#include <rscf/rscf_chassis_service.h>

#include "HC05.h"
#include "IMU.h"
#include "remote_control.h"
#include "robot_cmd.h"
#include "rscf_comm_service.h"
#include "robot_interface.h"
#include "rscf_imu_uart.h"

#define ROBOT_CMD_ACTION_STAGE_PERIOD_MS 250U
#define ROBOT_CMD_STATUS_PERIOD_MS 20U

typedef struct
{
    uint8_t inited;
    uint8_t ctrl_mode;
    uint8_t heading_mode;
    uint8_t velocity_frame;
    uint8_t last_service_id;
    uint8_t last_service_result;
    uint8_t last_service_ticket;
    uint8_t action_id;
    uint8_t action_state;
    uint8_t action_result;
    uint8_t action_ticket;
    uint8_t action_progress;
    uint8_t action_active;
    uint8_t action_phase;
    uint32_t action_tick;
    uint32_t last_status_tick;
    RobotArmState_e arm_state;
    RobotGimbalState_e gimbal_state;
    RobotBtState_e bt_state;
    RobotCommState_e comm_state;
} RobotCmdRuntime_s;

static RobotCmdRuntime_s s_robot_cmd = {
    .ctrl_mode = CONTROL_PLANE_CHASSIS_CTRL_MODE_CMD_VEL,
    .heading_mode = CONTROL_PLANE_HEADING_MODE_LOCK_ZERO,
    .velocity_frame = CONTROL_PLANE_VELOCITY_FRAME_WORLD,
    .last_service_id = CONTROL_PLANE_SERVICE_ID_NONE,
    .last_service_result = CONTROL_PLANE_SERVICE_RESULT_NONE,
    .action_id = CONTROL_PLANE_ACTION_ID_NONE,
    .action_state = CONTROL_PLANE_ACTION_STATE_IDLE,
    .action_result = CONTROL_PLANE_ACTION_RESULT_NONE,
    .arm_state = ROBOT_ARM_STATE_IDLE,
    .gimbal_state = ROBOT_GIMBAL_STATE_DISABLED,
    .bt_state = ROBOT_BT_STATE_DISCONNECTED,
    .comm_state = ROBOT_COMM_STATE_IDLE,
};

static uint8_t RobotCmdGetBits(const uint8_t *bytes, uint16_t bit_offset, uint8_t bit_width)
{
    uint8_t value = 0U;

    if ((bytes == NULL) || (bit_width == 0U))
    {
        return 0U;
    }

    for (uint8_t index = 0U; index < bit_width; ++index)
    {
        uint16_t absolute_bit = (uint16_t)(bit_offset + index);
        uint8_t byte_index = (uint8_t)(absolute_bit / 8U);
        uint8_t inner_bit = (uint8_t)(absolute_bit % 8U);
        value = (uint8_t)(value | (((bytes[byte_index] >> inner_bit) & 0x01U) << index));
    }

    return value;
}

static void RobotCmdSetBits(uint8_t *bytes, uint16_t bit_offset, uint8_t bit_width, uint8_t value)
{
    if ((bytes == NULL) || (bit_width == 0U))
    {
        return;
    }

    for (uint8_t index = 0U; index < bit_width; ++index)
    {
        uint16_t absolute_bit = (uint16_t)(bit_offset + index);
        uint8_t byte_index = (uint8_t)(absolute_bit / 8U);
        uint8_t inner_bit = (uint8_t)(absolute_bit % 8U);
        uint8_t bit_mask = (uint8_t)(0x01U << inner_bit);

        if (((value >> index) & 0x01U) != 0U)
        {
            bytes[byte_index] = (uint8_t)(bytes[byte_index] | bit_mask);
        }
        else
        {
            bytes[byte_index] = (uint8_t)(bytes[byte_index] & (uint8_t)(~bit_mask));
        }
    }
}

static void RobotCmdSetStatusField(uint16_t bit_offset, uint8_t bit_width, uint8_t value)
{
    RobotCmdSetBits(g_system_status.status_bytes, bit_offset, bit_width, value);
}

static void RobotCmdUpdateRoleState(void)
{
    s_robot_cmd.bt_state = HC05IsOnline() ? ROBOT_BT_STATE_LINKED : ROBOT_BT_STATE_DISCONNECTED;
    s_robot_cmd.comm_state = RobotIsConnected() ? ROBOT_COMM_STATE_USB_ACTIVE : ROBOT_COMM_STATE_IDLE;
    s_robot_cmd.gimbal_state = RSCFImuUartReady() ? ROBOT_GIMBAL_STATE_STABILIZE : ROBOT_GIMBAL_STATE_DISABLED;
    if (s_robot_cmd.action_active != 0U)
    {
        s_robot_cmd.arm_state = ROBOT_ARM_STATE_EXECUTING;
    }
    else if (RemoteControlIsOnline() != 0U)
    {
        s_robot_cmd.arm_state = ROBOT_ARM_STATE_HOLDING;
    }
    else
    {
        s_robot_cmd.arm_state = ROBOT_ARM_STATE_IDLE;
    }
}

static void RobotCmdRefreshBaseStatus(void)
{
    RobotCmdUpdateRoleState();
    s_robot_cmd.ctrl_mode = RSCFChassisServiceGetControlMode();
    s_robot_cmd.heading_mode = RSCFChassisServiceGetHeadingMode();
    s_robot_cmd.velocity_frame = RSCFChassisServiceGetVelocityFrame();

    RobotCmdSetStatusField(SYSTEM_STATUS_INIT_DONE_OFFSET, SYSTEM_STATUS_INIT_DONE_WIDTH, s_robot_cmd.inited);
    RobotCmdSetStatusField(SYSTEM_STATUS_ODOM_OK_OFFSET, SYSTEM_STATUS_ODOM_OK_WIDTH,
                           RSCFChassisServiceAnyMotorOnline() ? 1U : 0U);
    RobotCmdSetStatusField(SYSTEM_STATUS_TASK_ALL_OK_OFFSET, SYSTEM_STATUS_TASK_ALL_OK_WIDTH, 1U);
    RobotCmdSetStatusField(SYSTEM_STATUS_PEER_MCU_COMM_OK_OFFSET, SYSTEM_STATUS_PEER_MCU_COMM_OK_WIDTH,
                           RSCFCommServiceReady() ? 1U : 0U);
    RobotCmdSetStatusField(SYSTEM_STATUS_IMU_STATE_OFFSET, SYSTEM_STATUS_IMU_STATE_WIDTH,
                           RSCFImuUartReady() ? CONTROL_PLANE_IMU_STATE_OK : CONTROL_PLANE_IMU_STATE_DISABLED);
    RobotCmdSetStatusField(SYSTEM_STATUS_CTRL_MODE_OFFSET, SYSTEM_STATUS_CTRL_MODE_WIDTH, s_robot_cmd.ctrl_mode);
    RobotCmdSetStatusField(SYSTEM_STATUS_HEADING_MODE_OFFSET, SYSTEM_STATUS_HEADING_MODE_WIDTH, s_robot_cmd.heading_mode);
    RobotCmdSetStatusField(SYSTEM_STATUS_VELOCITY_FRAME_OFFSET, SYSTEM_STATUS_VELOCITY_FRAME_WIDTH,
                           s_robot_cmd.velocity_frame);
    RobotCmdSetStatusField(SYSTEM_STATUS_SERVICE_ID_OFFSET, SYSTEM_STATUS_SERVICE_ID_WIDTH, s_robot_cmd.last_service_id);
    RobotCmdSetStatusField(SYSTEM_STATUS_SERVICE_RESULT_OFFSET, SYSTEM_STATUS_SERVICE_RESULT_WIDTH,
                           s_robot_cmd.last_service_result);
    RobotCmdSetStatusField(SYSTEM_STATUS_SERVICE_TICKET_OFFSET, SYSTEM_STATUS_SERVICE_TICKET_WIDTH,
                           s_robot_cmd.last_service_ticket);
    RobotCmdSetStatusField(SYSTEM_STATUS_ACTION_ID_OFFSET, SYSTEM_STATUS_ACTION_ID_WIDTH, s_robot_cmd.action_id);
    RobotCmdSetStatusField(SYSTEM_STATUS_ACTION_STATE_OFFSET, SYSTEM_STATUS_ACTION_STATE_WIDTH, s_robot_cmd.action_state);
    RobotCmdSetStatusField(SYSTEM_STATUS_ACTION_TICKET_OFFSET, SYSTEM_STATUS_ACTION_TICKET_WIDTH, s_robot_cmd.action_ticket);
    RobotCmdSetStatusField(SYSTEM_STATUS_ACTION_RESULT_OFFSET, SYSTEM_STATUS_ACTION_RESULT_WIDTH, s_robot_cmd.action_result);
    RobotCmdSetStatusField(SYSTEM_STATUS_ACTION_PROGRESS_OFFSET, SYSTEM_STATUS_ACTION_PROGRESS_WIDTH,
                           s_robot_cmd.action_progress);
}

static void RobotCmdFinishAction(uint8_t state, uint8_t result, uint8_t progress)
{
    s_robot_cmd.action_state = state;
    s_robot_cmd.action_result = result;
    s_robot_cmd.action_progress = progress;
    s_robot_cmd.action_active = 0U;
    s_robot_cmd.action_phase = 0U;
    RobotInterfaceRequestTxEvent(ROBOT_INTERFACE_TX_EVENT_SYSTEM_STATUS);
}

void RobotCmdInit(void)
{
    memset(g_system_status.status_bytes, 0, sizeof(g_system_status.status_bytes));
    s_robot_cmd.inited = 1U;
    s_robot_cmd.last_service_id = CONTROL_PLANE_SERVICE_ID_NONE;
    s_robot_cmd.last_service_result = CONTROL_PLANE_SERVICE_RESULT_NONE;
    s_robot_cmd.last_service_ticket = 0U;
    s_robot_cmd.action_id = CONTROL_PLANE_ACTION_ID_NONE;
    s_robot_cmd.action_state = CONTROL_PLANE_ACTION_STATE_IDLE;
    s_robot_cmd.action_result = CONTROL_PLANE_ACTION_RESULT_NONE;
    s_robot_cmd.action_ticket = 0U;
    s_robot_cmd.action_progress = 0U;
    RobotCmdRefreshBaseStatus();
}

void RobotCmdTask(void)
{
    uint32_t now_tick = k_uptime_get_32();

    RobotCmdRefreshBaseStatus();
    if ((s_robot_cmd.action_active == 0U) ||
        ((uint32_t)(now_tick - s_robot_cmd.action_tick) < ROBOT_CMD_ACTION_STAGE_PERIOD_MS))
    {
        return;
    }

    s_robot_cmd.action_tick = now_tick;
    s_robot_cmd.action_phase++;
    s_robot_cmd.action_state = CONTROL_PLANE_ACTION_STATE_RUNNING;
    s_robot_cmd.action_progress = (uint8_t)(s_robot_cmd.action_phase * 20U);

    if (s_robot_cmd.action_id == CONTROL_PLANE_ACTION_ID_STEP_TRANSITION)
    {
        s_robot_cmd.gimbal_state = ROBOT_GIMBAL_STATE_STABILIZE;
    }
    else
    {
        s_robot_cmd.gimbal_state = ROBOT_GIMBAL_STATE_TRACK;
    }

    if (s_robot_cmd.action_phase >= 5U)
    {
        RobotCmdFinishAction(CONTROL_PLANE_ACTION_STATE_SUCCEEDED,
                             CONTROL_PLANE_ACTION_RESULT_OK,
                             63U);
    }
}

void RobotCmdCommPublishTick(void)
{
    uint32_t now_tick = k_uptime_get_32();

    if ((uint32_t)(now_tick - s_robot_cmd.last_status_tick) < ROBOT_CMD_STATUS_PERIOD_MS)
    {
        return;
    }

    s_robot_cmd.last_status_tick = now_tick;
    RobotCmdRefreshBaseStatus();
    (void)RobotSendSystemStatus(g_system_status.status_bytes);
}

void RobotCmdCommPublishNow(void)
{
    RobotCmdRefreshBaseStatus();
    (void)RobotSendSystemStatus(g_system_status.status_bytes);
}

void RobotCmdHandleSystemCmd(const systemCmd_t *cmd)
{
    uint8_t service_id;
    uint8_t service_arg;
    uint8_t service_ticket;
    uint8_t action_id;
    uint8_t action_cmd;
    uint8_t action_ticket;

    if (cmd == NULL)
    {
        cmd = &g_system_cmd;
    }

    service_id = RobotCmdGetBits(cmd->cmd_bytes, SYSTEM_CMD_SERVICE_ID_OFFSET, SYSTEM_CMD_SERVICE_ID_WIDTH);
    service_arg = RobotCmdGetBits(cmd->cmd_bytes, SYSTEM_CMD_SERVICE_ARG_OFFSET, SYSTEM_CMD_SERVICE_ARG_WIDTH);
    service_ticket = RobotCmdGetBits(cmd->cmd_bytes, SYSTEM_CMD_SERVICE_TICKET_OFFSET, SYSTEM_CMD_SERVICE_TICKET_WIDTH);
    action_id = RobotCmdGetBits(cmd->cmd_bytes, SYSTEM_CMD_ACTION_ID_OFFSET, SYSTEM_CMD_ACTION_ID_WIDTH);
    action_cmd = RobotCmdGetBits(cmd->cmd_bytes, SYSTEM_CMD_ACTION_CMD_OFFSET, SYSTEM_CMD_ACTION_CMD_WIDTH);
    action_ticket = RobotCmdGetBits(cmd->cmd_bytes, SYSTEM_CMD_ACTION_TICKET_OFFSET, SYSTEM_CMD_ACTION_TICKET_WIDTH);

    s_robot_cmd.last_service_id = service_id;
    s_robot_cmd.last_service_ticket = service_ticket;
    s_robot_cmd.last_service_result = CONTROL_PLANE_SERVICE_RESULT_REJECT;

    switch (service_id)
    {
    case CONTROL_PLANE_SERVICE_ID_SET_CTRL_MODE:
        if (service_arg <= CONTROL_PLANE_CHASSIS_CTRL_MODE_BRAKE_STOP)
        {
            RSCFChassisServiceSetControlMode(service_arg);
            s_robot_cmd.last_service_result = CONTROL_PLANE_SERVICE_RESULT_OK;
        }
        break;

    case CONTROL_PLANE_SERVICE_ID_SET_HEADING_MODE:
        if (service_arg <= CONTROL_PLANE_HEADING_MODE_FOLLOW_TARGET)
        {
            RSCFChassisServiceSetHeadingMode(service_arg);
            s_robot_cmd.last_service_result = CONTROL_PLANE_SERVICE_RESULT_OK;
        }
        break;

    case CONTROL_PLANE_SERVICE_ID_SET_VELOCITY_FRAME:
        if (service_arg <= CONTROL_PLANE_VELOCITY_FRAME_BODY)
        {
            RSCFChassisServiceSetVelocityFrame(service_arg);
            s_robot_cmd.last_service_result = CONTROL_PLANE_SERVICE_RESULT_OK;
        }
        break;

    case CONTROL_PLANE_SERVICE_ID_NONE:
    default:
        break;
    }

    if (action_id != CONTROL_PLANE_ACTION_ID_NONE)
    {
        if (action_cmd == CONTROL_PLANE_ACTION_CMD_CANCEL)
        {
            s_robot_cmd.action_id = action_id;
            s_robot_cmd.action_ticket = action_ticket;
            RobotCmdFinishAction(CONTROL_PLANE_ACTION_STATE_CANCELED,
                                 CONTROL_PLANE_ACTION_RESULT_FAIL,
                                 0U);
        }
        else if (action_cmd == CONTROL_PLANE_ACTION_CMD_START)
        {
            s_robot_cmd.action_id = action_id;
            s_robot_cmd.action_ticket = action_ticket;
            s_robot_cmd.action_result = CONTROL_PLANE_ACTION_RESULT_NONE;
            s_robot_cmd.action_state = CONTROL_PLANE_ACTION_STATE_ACCEPTED;
            s_robot_cmd.action_progress = 1U;
            s_robot_cmd.action_active = 1U;
            s_robot_cmd.action_phase = 0U;
            s_robot_cmd.action_tick = k_uptime_get_32();
        }
    }

    RobotInterfaceRequestTxEvent(ROBOT_INTERFACE_TX_EVENT_SYSTEM_STATUS);
}

RobotArmState_e RobotCmdGetArmState(void)
{
    return s_robot_cmd.arm_state;
}

RobotGimbalState_e RobotCmdGetGimbalState(void)
{
    return s_robot_cmd.gimbal_state;
}

RobotBtState_e RobotCmdGetBtState(void)
{
    return s_robot_cmd.bt_state;
}

RobotCommState_e RobotCmdGetCommState(void)
{
    return s_robot_cmd.comm_state;
}

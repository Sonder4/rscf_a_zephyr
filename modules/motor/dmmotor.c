#include <stdlib.h>
#include <string.h>

#include "bsp_dwt.h"
#include "bsp_log.h"
#include "dmmotor.h"
#include "general_def.h"
#include "unit_convert.h"

static uint8_t s_dm_motor_count;
static DMMotorInstance *s_dm_motor_instance[DM_MOTOR_CNT];

static void DMMotorNormalizePhyCfg(Motor_Type_e motor_type, MotorPhyCfg_t *phy_cfg)
{
    (void)motor_type;

    if (phy_cfg == NULL)
    {
        return;
    }

    if (phy_cfg->ratio <= 0.0f)
    {
        phy_cfg->ratio = 1.0f;
    }
}

static uint16_t float_to_uint(float x, float x_min, float x_max, uint8_t bits)
{
    float span = x_max - x_min;
    float offset = x_min;
    return (uint16_t)((x - offset) * ((float)((1U << bits) - 1U)) / span);
}

static float uint_to_float(int x_int, float x_min, float x_max, int bits)
{
    float span = x_max - x_min;
    float offset = x_min;
    return ((float)x_int) * span / ((float)((1U << bits) - 1U)) + offset;
}

static void DMMotorSetMode(DMMotor_Mode_e cmd, DMMotorInstance *motor)
{
    if ((motor == NULL) || (motor->motor_can_instace == NULL))
    {
        return;
    }

    memset(motor->motor_can_instace->tx_buff, 0xFF, 7U);
    motor->motor_can_instace->tx_buff[7] = (uint8_t)cmd;
    (void)CANTransmit(motor->motor_can_instace, 1.0f);
}

static void DMMotorDecode(CANInstance *motor_can)
{
    uint16_t tmp;
    uint8_t *rxbuff;
    DMMotorInstance *motor;
    DM_Motor_Measure_s *measure;

    if ((motor_can == NULL) || (motor_can->id == NULL))
    {
        return;
    }

    rxbuff = motor_can->rx_buff;
    motor = (DMMotorInstance *)motor_can->id;
    measure = &motor->measure;

    DaemonReload(motor->motor_daemon);
    measure->last_position = measure->position;
    tmp = (uint16_t)((rxbuff[1] << 8) | rxbuff[2]);
    measure->position = uint_to_float(tmp, DM_P_MIN, DM_P_MAX, 16);
    tmp = (uint16_t)((rxbuff[3] << 4) | (rxbuff[4] >> 4));
    measure->velocity = uint_to_float(tmp, DM_V_MIN, DM_V_MAX, 12);
    tmp = (uint16_t)(((rxbuff[4] & 0x0F) << 8) | rxbuff[5]);
    measure->torque = uint_to_float(tmp, DM_T_MIN, DM_T_MAX, 12);
    measure->T_Mos = (float)rxbuff[6];
    measure->T_Rotor = (float)rxbuff[7];

    motor->fb.out.pos_rad = measure->position;
    motor->fb.out.vel_radps = measure->velocity;
    motor->fb.out.tor_nm = measure->torque;
    motor->fb.out.tmp_c = measure->T_Mos;
    motor->fb.out.cur_ma = (motor->phy_cfg.tor_nm_per_ma > 0.0f) ?
        (measure->torque / motor->phy_cfg.tor_nm_per_ma) : 0.0f;
    motor->fb.has_out = 1U;
    motor->fb.has_rot = 0U;
}

static void DMMotorDaemonCallback(void *motor_ptr, DaemonEvent_e event)
{
    DMMotorInstance *motor = (DMMotorInstance *)motor_ptr;

    if (motor == NULL)
    {
        return;
    }

    if (event == DAEMON_EVENT_OFFLINE_ENTER)
    {
        LOGWARNING("[dm_motor] motor %d lost", motor->motor_can_instace->tx_id);
    }
    else if (event == DAEMON_EVENT_OFFLINE_REMIND)
    {
        LOGWARNING("[dm_motor] motor %d still lost", motor->motor_can_instace->tx_id);
    }
    else if (event == DAEMON_EVENT_ONLINE_RECOVER)
    {
        LOGINFO("[dm_motor] motor %d recovered", motor->motor_can_instace->tx_id);
    }
}

void DMMotorCaliEncoder(DMMotorInstance *motor)
{
    DMMotorSetMode(DM_CMD_ZERO_POSITION, motor);
    DWT_Delay(0.1f);
}

DMMotorInstance *DMMotorInit(Motor_Init_Config_s *config)
{
    DMMotorInstance *motor;
    Daemon_Init_Config_s conf;

    if ((config == NULL) || (s_dm_motor_count >= DM_MOTOR_CNT))
    {
        return NULL;
    }

    motor = (DMMotorInstance *)malloc(sizeof(DMMotorInstance));
    if (motor == NULL)
    {
        return NULL;
    }

    memset(motor, 0, sizeof(DMMotorInstance));
    motor->motor_settings = config->controller_setting_init_config;
    motor->phy_cfg = config->phy_cfg;
    motor->phy_cfg.raw_is_out = 1U;
    DMMotorNormalizePhyCfg(config->motor_type, &motor->phy_cfg);
    PIDInit(&motor->current_PID, &config->controller_param_init_config.current_PID);
    PIDInit(&motor->speed_PID, &config->controller_param_init_config.speed_PID);
    PIDInit(&motor->angle_PID, &config->controller_param_init_config.angle_PID);
    motor->other_angle_feedback_ptr = config->controller_param_init_config.other_angle_feedback_ptr;
    motor->other_speed_feedback_ptr = config->controller_param_init_config.other_speed_feedback_ptr;

    config->can_init_config.can_module_callback = DMMotorDecode;
    config->can_init_config.id = motor;
    motor->motor_can_instace = CANRegister(&config->can_init_config);
    if (motor->motor_can_instace == NULL)
    {
        free(motor);
        return NULL;
    }

    conf.callback = DMMotorDaemonCallback;
    conf.owner_id = motor;
    conf.reload_count = 10U;
    conf.init_count = 0U;
    conf.remind_count = DAEMON_REMIND_15S;
    motor->motor_daemon = DaemonRegister(&conf);

    DMMotorEnable(motor);
    DMMotorSetMode(DM_CMD_MOTOR_MODE, motor);
    DWT_Delay(0.1f);
    DMMotorCaliEncoder(motor);
    DWT_Delay(0.1f);
    s_dm_motor_instance[s_dm_motor_count++] = motor;
    return motor;
}

void DMMotorSetRef(DMMotorInstance *motor, float ref)
{
    if (motor != NULL)
    {
        motor->pid_ref = ref;
    }
}

void DMMotorEnable(DMMotorInstance *motor)
{
    if (motor != NULL)
    {
        motor->stop_flag = MOTOR_ENALBED;
    }
}

void DMMotorStop(DMMotorInstance *motor)
{
    if (motor != NULL)
    {
        motor->stop_flag = MOTOR_STOP;
    }
}

void DMMotorOuterLoop(DMMotorInstance *motor, Closeloop_Type_e type)
{
    if (motor != NULL)
    {
        motor->motor_settings.outer_loop_type = type;
    }
}

void DMMotorControlInit(void)
{
    /* Zephyr 版本由统一的 motor service 周期调度，这里保留兼容接口。 */
}

void DMMotorControl(void)
{
    for (size_t i = 0; i < s_dm_motor_count; ++i)
    {
        DMMotorInstance *motor = s_dm_motor_instance[i];
        DMMotor_Send_s mailbox;
        float set = motor->pid_ref;

        if (motor->motor_settings.motor_reverse_flag == MOTOR_DIRECTION_REVERSE)
        {
            set *= -1.0f;
        }

        LIMIT_MIN_MAX(set, DM_T_MIN, DM_T_MAX);
        mailbox.position_des = float_to_uint(0.0f, DM_P_MIN, DM_P_MAX, 16);
        mailbox.velocity_des = float_to_uint(0.0f, DM_V_MIN, DM_V_MAX, 12);
        mailbox.torque_des = float_to_uint((motor->stop_flag == MOTOR_STOP) ? 0.0f : set, DM_T_MIN, DM_T_MAX, 12);
        mailbox.Kp = 0U;
        mailbox.Kd = 0U;

        motor->motor_can_instace->tx_buff[0] = (uint8_t)(mailbox.position_des >> 8);
        motor->motor_can_instace->tx_buff[1] = (uint8_t)(mailbox.position_des);
        motor->motor_can_instace->tx_buff[2] = (uint8_t)(mailbox.velocity_des >> 4);
        motor->motor_can_instace->tx_buff[3] = (uint8_t)(((mailbox.velocity_des & 0x0F) << 4) | (mailbox.Kp >> 8));
        motor->motor_can_instace->tx_buff[4] = (uint8_t)(mailbox.Kp);
        motor->motor_can_instace->tx_buff[5] = (uint8_t)(mailbox.Kd >> 4);
        motor->motor_can_instace->tx_buff[6] = (uint8_t)(((mailbox.Kd & 0x0F) << 4) | (mailbox.torque_des >> 8));
        motor->motor_can_instace->tx_buff[7] = (uint8_t)(mailbox.torque_des);
        (void)CANTransmitTry(motor->motor_can_instace);
    }
}

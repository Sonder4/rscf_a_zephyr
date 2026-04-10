#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "bsp_dwt.h"
#include "bsp_log.h"
#include "dji_motor.h"
#include "general_def.h"

static uint8_t s_dji_motor_count;
static DJIMotorInstance *s_dji_motor_instance[DJI_MOTOR_CNT];
static const uint8_t s_freq_period_ms[4] = {1U, 2U, 5U, 10U};

static CANInstance s_sender_assignment[6] = {
    [0] = {.can_handle = &hcan1, .txconf = {.StdId = 0x1ff, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 0x08}, .tx_id = 0x1ff},
    [1] = {.can_handle = &hcan1, .txconf = {.StdId = 0x200, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 0x08}, .tx_id = 0x200},
    [2] = {.can_handle = &hcan1, .txconf = {.StdId = 0x2ff, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 0x08}, .tx_id = 0x2ff},
    [3] = {.can_handle = &hcan2, .txconf = {.StdId = 0x1ff, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 0x08}, .tx_id = 0x1ff},
    [4] = {.can_handle = &hcan2, .txconf = {.StdId = 0x200, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 0x08}, .tx_id = 0x200},
    [5] = {.can_handle = &hcan2, .txconf = {.StdId = 0x2ff, .IDE = CAN_ID_STD, .RTR = CAN_RTR_DATA, .DLC = 0x08}, .tx_id = 0x2ff},
};
static uint8_t s_sender_enable_flag[6];

#define DJI_GM6020_RAW_CURRENT_SAFE_LIMIT 25000.0f
#define DJI_M3508_MAX_ROTOR_SPEED_RPM 9000.0f
#define DJI_M2006_MAX_ROTOR_SPEED_RPM 16000.0f

static int16_t DJIMotorEncodeControlOutput(const DJIMotorInstance *motor, float control_output);
static void DecodeDJIMotor(CANInstance *_instance);

float DJIMotorGetDefaultRatio(Motor_Type_e motor_type)
{
    switch (motor_type)
    {
    case M3508:
        return 19.21f;
    case M2006:
        return 36.0f;
    case GM6020:
    default:
        return 1.0f;
    }
}

float DJIMotorGetDefaultCurMaPerRaw(Motor_Type_e motor_type)
{
    switch (motor_type)
    {
    case M3508:
        return 20000.0f / 16384.0f;
    case M2006:
        return 1.0f;
    case GM6020:
    default:
        return 0.0f;
    }
}

float DJIMotorGetDefaultTorNmPerMa(Motor_Type_e motor_type)
{
    switch (motor_type)
    {
    case M3508:
        return 0.3f / 1000.0f;
    case M2006:
        return 0.18f / 1000.0f;
    case GM6020:
        return 0.000741f;
    default:
        return 0.0f;
    }
}

void DJIMotorNormalizePhyCfg(Motor_Type_e motor_type, MotorPhyCfg_t *phy_cfg)
{
    if (phy_cfg == NULL)
    {
        return;
    }

    if (phy_cfg->ratio <= 0.0f)
    {
        phy_cfg->ratio = DJIMotorGetDefaultRatio(motor_type);
    }
    if (phy_cfg->cur_ma_per_raw <= 0.0f)
    {
        phy_cfg->cur_ma_per_raw = DJIMotorGetDefaultCurMaPerRaw(motor_type);
    }
    if (phy_cfg->tor_nm_per_ma <= 0.0f)
    {
        phy_cfg->tor_nm_per_ma = DJIMotorGetDefaultTorNmPerMa(motor_type);
    }
}

static float DJIMotorGetRotorSpeedLimitRadps(Motor_Type_e motor_type)
{
    switch (motor_type)
    {
    case M3508:
        return UnitRpmToRadps(DJI_M3508_MAX_ROTOR_SPEED_RPM);
    case M2006:
        return UnitRpmToRadps(DJI_M2006_MAX_ROTOR_SPEED_RPM);
    default:
        return 0.0f;
    }
}

static uint8_t DJIMotorUsesRawTorqueCurrent(const DJIMotorInstance *motor)
{
    return ((motor != NULL) && (motor->motor_type == GM6020)) ? 1U : 0U;
}

static float DJIMotorClampSpeedRef(const DJIMotorInstance *motor, float speed_ref_radps)
{
    float rotor_limit_radps;
    float output_limit_radps;

    if (motor == NULL)
    {
        return speed_ref_radps;
    }

    rotor_limit_radps = DJIMotorGetRotorSpeedLimitRadps(motor->motor_type);
    if (rotor_limit_radps <= 0.0f)
    {
        return speed_ref_radps;
    }

    output_limit_radps = UnitRotorToOutputVelRadps(rotor_limit_radps, motor->phy_cfg.ratio);
    LIMIT_MIN_MAX(speed_ref_radps, -output_limit_radps, output_limit_radps);
    return speed_ref_radps;
}

static int16_t DJIMotorCurrentMaToRaw(const DJIMotorInstance *motor, float current_ma)
{
    if ((motor == NULL) || (motor->phy_cfg.cur_ma_per_raw <= 0.0f))
    {
        return (int16_t)current_ma;
    }

    return (int16_t)(current_ma / motor->phy_cfg.cur_ma_per_raw);
}

static uint8_t DJIMotorDecodeTemperature(const DJIMotorInstance *motor, const uint8_t *rx_buff)
{
    if ((motor == NULL) || (rx_buff == NULL))
    {
        return 0U;
    }

    switch (motor->motor_type)
    {
    case M3508:
        return rx_buff[6];
    case GM6020:
        return rx_buff[7];
    case M2006:
    default:
        return 0U;
    }
}

static int16_t DJIMotorEncodeControlOutput(const DJIMotorInstance *motor, float control_output)
{
    if (DJIMotorUsesRawTorqueCurrent(motor) != 0U)
    {
        LIMIT_MIN_MAX(control_output, -DJI_GM6020_RAW_CURRENT_SAFE_LIMIT, DJI_GM6020_RAW_CURRENT_SAFE_LIMIT);
        return (int16_t)control_output;
    }

    return DJIMotorCurrentMaToRaw(motor, control_output);
}

static void DJIMotorUpdateFeedback(DJIMotorInstance *motor)
{
    DJI_Motor_Measure_s *measure;
    int32_t delta_ecd;
    float rot_pos_single_rad;
    float rot_vel_radps;
    float cur_ma;
    float out_tor_nm;

    if (motor == NULL)
    {
        return;
    }

    measure = &motor->measure;
    delta_ecd = (int32_t)measure->ecd - (int32_t)measure->last_ecd;
    if (delta_ecd > 4096)
    {
        motor->total_round--;
    }
    else if (delta_ecd < -4096)
    {
        motor->total_round++;
    }

    rot_pos_single_rad = ((float)measure->ecd / 8192.0f) * TWO_PI;
    rot_vel_radps = UnitRpmToRadps((float)measure->speed_rpm);
    cur_ma = (DJIMotorUsesRawTorqueCurrent(motor) != 0U) ? 0.0f :
        UnitRawCurrentToMa((float)measure->real_current, motor->phy_cfg.cur_ma_per_raw);

    motor->fb.rot.pos_rad = (float)motor->total_round * TWO_PI + rot_pos_single_rad;
    motor->fb.rot.vel_radps = UnitLpf1Update(&motor->vel_lpf, rot_vel_radps);
    motor->fb.rot.cur_ma = UnitLpf1Update(&motor->cur_lpf, cur_ma);
    motor->fb.rot.tmp_c = (float)measure->temperature;
    motor->fb.out.pos_rad = UnitRotorToOutputPosRad(motor->fb.rot.pos_rad, motor->phy_cfg.ratio);
    motor->fb.out.vel_radps = UnitRotorToOutputVelRadps(motor->fb.rot.vel_radps, motor->phy_cfg.ratio);
    motor->fb.out.cur_ma = motor->fb.rot.cur_ma;
    motor->fb.out.tmp_c = motor->fb.rot.tmp_c;

    if (DJIMotorUsesRawTorqueCurrent(motor) != 0U)
    {
        motor->fb.out.tor_nm = 0.0f;
        motor->fb.rot.tor_nm = 0.0f;
    }
    else
    {
        out_tor_nm = UnitCurrentMaToTorqueNm(motor->fb.out.cur_ma, motor->phy_cfg.tor_nm_per_ma);
        motor->fb.out.tor_nm = UnitLpf1Update(&motor->tor_lpf, out_tor_nm);
        motor->fb.rot.tor_nm = (motor->phy_cfg.ratio > 0.0f) ?
            (motor->fb.out.tor_nm / motor->phy_cfg.ratio) : motor->fb.out.tor_nm;
    }

    motor->fb.has_rot = 1U;
    motor->fb.has_out = 1U;
}

static void MotorSenderGrouping(DJIMotorInstance *motor, CAN_Init_Config_s *config)
{
    uint8_t motor_id;
    uint8_t motor_send_num;
    uint8_t motor_grouping;
    uint8_t grouping_offset;

    if ((motor == NULL) || (config == NULL) || (config->tx_id == 0U))
    {
        return;
    }

    motor_id = (uint8_t)(config->tx_id - 1U);
    grouping_offset = (config->can_handle == &hcan2) ? 3U : 0U;

    switch (motor->motor_type)
    {
    case M2006:
    case M3508:
        if (motor_id < 4U)
        {
            motor_send_num = motor_id;
            motor_grouping = grouping_offset + 1U;
        }
        else
        {
            motor_send_num = (uint8_t)(motor_id - 4U);
            motor_grouping = grouping_offset;
        }
        config->rx_id = 0x200U + motor_id + 1U;
        break;
    case GM6020:
        if (motor_id < 4U)
        {
            motor_send_num = motor_id;
            motor_grouping = grouping_offset;
        }
        else
        {
            motor_send_num = (uint8_t)(motor_id - 4U);
            motor_grouping = grouping_offset + 2U;
        }
        config->rx_id = 0x204U + motor_id + 1U;
        break;
    default:
        LOGERROR("[dji_motor] unsupported motor type");
        return;
    }

    s_sender_enable_flag[motor_grouping] = 1U;
    motor->message_num = motor_send_num;
    motor->sender_group = motor_grouping;
}

static void DecodeDJIMotor(CANInstance *_instance)
{
    uint8_t *rxbuff;
    DJIMotorInstance *motor;
    DJI_Motor_Measure_s *measure;

    if ((_instance == NULL) || (_instance->id == NULL))
    {
        return;
    }

    rxbuff = _instance->rx_buff;
    motor = (DJIMotorInstance *)_instance->id;
    measure = &motor->measure;

    DaemonReload(motor->daemon);
    motor->dt = DWT_GetDeltaT(&motor->feed_cnt);
    DJIMotorEnable(motor);

    measure->last_ecd = measure->ecd;
    measure->ecd = ((uint16_t)rxbuff[0] << 8) | rxbuff[1];
    measure->speed_rpm = (int16_t)((rxbuff[2] << 8) | rxbuff[3]);
    measure->real_current = (int16_t)((rxbuff[4] << 8) | rxbuff[5]);
    measure->temperature = DJIMotorDecodeTemperature(motor, rxbuff);

    DJIMotorUpdateFeedback(motor);
}

static void DJIMotorDaemonCallback(void *motor_ptr, DaemonEvent_e event)
{
    DJIMotorInstance *motor = (DJIMotorInstance *)motor_ptr;
    uint16_t can_bus = (motor->motor_can_instance->can_handle == &hcan2) ? 2U : 1U;

    if (event == DAEMON_EVENT_OFFLINE_ENTER)
    {
        DJIMotorStop(motor);
        LOGWARNING("[dji_motor] motor lost, can bus [%d], id [%d]", can_bus, motor->motor_can_instance->tx_id);
    }
    else if (event == DAEMON_EVENT_OFFLINE_REMIND)
    {
        LOGWARNING("[dji_motor] motor still lost, can bus [%d], id [%d]", can_bus, motor->motor_can_instance->tx_id);
    }
    else if (event == DAEMON_EVENT_ONLINE_RECOVER)
    {
        LOGINFO("[dji_motor] motor recovered, can bus [%d], id [%d]", can_bus, motor->motor_can_instance->tx_id);
    }
}

DJIMotorInstance *DJIMotorInit(Motor_Init_Config_s *config)
{
    DJIMotorInstance *instance;
    Daemon_Init_Config_s daemon_config;

    if ((config == NULL) || (s_dji_motor_count >= DJI_MOTOR_CNT))
    {
        return NULL;
    }

    instance = (DJIMotorInstance *)malloc(sizeof(DJIMotorInstance));
    if (instance == NULL)
    {
        return NULL;
    }

    memset(instance, 0, sizeof(DJIMotorInstance));
    instance->motor_type = config->motor_type;
    instance->motor_settings = config->controller_setting_init_config;
    instance->phy_cfg = config->phy_cfg;
    DJIMotorNormalizePhyCfg(instance->motor_type, &instance->phy_cfg);
    UnitLpf1Init(&instance->vel_lpf, SPEED_SMOOTH_COEF);
    UnitLpf1Init(&instance->cur_lpf, CURRENT_SMOOTH_COEF);
    UnitLpf1Init(&instance->tor_lpf, CURRENT_SMOOTH_COEF);

    PIDInit(&instance->motor_controller.current_PID, &config->controller_param_init_config.current_PID);
    PIDInit(&instance->motor_controller.speed_PID, &config->controller_param_init_config.speed_PID);
    PIDInit(&instance->motor_controller.angle_PID, &config->controller_param_init_config.angle_PID);
    instance->motor_controller.other_angle_feedback_ptr = config->controller_param_init_config.other_angle_feedback_ptr;
    instance->motor_controller.other_speed_feedback_ptr = config->controller_param_init_config.other_speed_feedback_ptr;
    instance->motor_controller.current_feedforward_ptr = config->controller_param_init_config.current_feedforward_ptr;
    instance->motor_controller.speed_feedforward_ptr = config->controller_param_init_config.speed_feedforward_ptr;
    instance->motor_controller.control_freq = MOTOR_FREQ_1000HZ;
    instance->motor_controller.control_cache = 0;

    MotorSenderGrouping(instance, &config->can_init_config);
    config->can_init_config.can_module_callback = DecodeDJIMotor;
    config->can_init_config.id = instance;
    instance->motor_can_instance = CANRegister(&config->can_init_config);
    if (instance->motor_can_instance == NULL)
    {
        free(instance);
        return NULL;
    }

    daemon_config.callback = DJIMotorDaemonCallback;
    daemon_config.owner_id = instance;
    daemon_config.reload_count = 2U;
    daemon_config.init_count = 0U;
    daemon_config.remind_count = DAEMON_REMIND_15S;
    instance->daemon = DaemonRegister(&daemon_config);

    DJIMotorEnable(instance);
    s_dji_motor_instance[s_dji_motor_count++] = instance;
    return instance;
}

void DJIMotorChangeFeed(DJIMotorInstance *motor, Closeloop_Type_e loop, Feedback_Source_e type)
{
    if (motor == NULL)
    {
        return;
    }

    if (loop == ANGLE_LOOP)
    {
        motor->motor_settings.angle_feedback_source = type;
    }
    else if (loop == SPEED_LOOP)
    {
        motor->motor_settings.speed_feedback_source = type;
    }
}

void DJIMotorStop(DJIMotorInstance *motor)
{
    if (motor == NULL)
    {
        return;
    }

    motor->measure.speed_rpm = 0;
    motor->measure.real_current = 0;
    motor->fb.rot.vel_radps = 0.0f;
    motor->fb.rot.cur_ma = 0.0f;
    motor->fb.rot.tor_nm = 0.0f;
    motor->fb.out.vel_radps = 0.0f;
    motor->fb.out.cur_ma = 0.0f;
    motor->fb.out.tor_nm = 0.0f;
    motor->stop_flag = MOTOR_STOP;
}

void DJIMotorEnable(DJIMotorInstance *motor)
{
    if (motor != NULL)
    {
        motor->stop_flag = MOTOR_ENALBED;
    }
}

void DJIMotorOuterLoop(DJIMotorInstance *motor, Closeloop_Type_e outer_loop)
{
    if (motor != NULL)
    {
        motor->motor_settings.outer_loop_type = outer_loop;
    }
}

void DJIMotorSetFreq(DJIMotorInstance *motor, Motor_Control_Freq_e freq)
{
    if (motor != NULL)
    {
        motor->motor_controller.control_freq = freq;
    }
}

void DJIMotorSetRef(DJIMotorInstance *motor, float ref)
{
    if (motor != NULL)
    {
        motor->motor_controller.pid_ref = ref;
    }
}

static int16_t DJIMotorCompute(DJIMotorInstance *motor)
{
    Motor_Control_Setting_s *motor_setting;
    Motor_Controller_s *motor_controller;
    float pid_measure;
    float pid_ref;

    if (motor == NULL)
    {
        return 0;
    }

    motor_setting = &motor->motor_settings;
    motor_controller = &motor->motor_controller;
    pid_ref = motor_controller->pid_ref;
    if (motor_setting->motor_reverse_flag == MOTOR_DIRECTION_REVERSE)
    {
        pid_ref *= -1.0f;
    }

    if ((motor_setting->close_loop_type & ANGLE_LOOP) && (motor_setting->outer_loop_type == ANGLE_LOOP))
    {
        pid_measure = (motor_setting->angle_feedback_source == OTHER_FEED && motor_controller->other_angle_feedback_ptr != NULL) ?
            *motor_controller->other_angle_feedback_ptr : motor->fb.out.pos_rad;
        pid_ref = PIDCalculate(&motor_controller->angle_PID, pid_measure, pid_ref);
    }

    if ((motor_setting->close_loop_type & SPEED_LOOP) &&
        (motor_setting->outer_loop_type & (ANGLE_LOOP | SPEED_LOOP)))
    {
        if ((motor_setting->feedforward_flag & SPEED_FEEDFORWARD) && (motor_controller->speed_feedforward_ptr != NULL))
        {
            pid_ref += *motor_controller->speed_feedforward_ptr;
        }

        pid_ref = DJIMotorClampSpeedRef(motor, pid_ref);
        pid_measure = (motor_setting->speed_feedback_source == OTHER_FEED && motor_controller->other_speed_feedback_ptr != NULL) ?
            *motor_controller->other_speed_feedback_ptr : motor->fb.out.vel_radps;
        pid_ref = PIDCalculate(&motor_controller->speed_PID, pid_measure, pid_ref);
    }

    if ((motor_setting->feedforward_flag & CURRENT_FEEDFORWARD) && (motor_controller->current_feedforward_ptr != NULL))
    {
        pid_ref += *motor_controller->current_feedforward_ptr;
    }

    if ((motor_setting->close_loop_type & CURRENT_LOOP) && (DJIMotorUsesRawTorqueCurrent(motor) == 0U))
    {
        pid_ref = PIDCalculate(&motor_controller->current_PID, motor->fb.out.cur_ma, pid_ref);
    }

    if (motor_setting->feedback_reverse_flag == FEEDBACK_DIRECTION_REVERSE)
    {
        pid_ref *= -1.0f;
    }

    if (motor->stop_flag == MOTOR_STOP)
    {
        return 0;
    }

    return DJIMotorEncodeControlOutput(motor, pid_ref);
}

void DJIMotorControl(void)
{
    static uint8_t tick_counter[DJI_MOTOR_CNT];
    uint8_t group;
    uint8_t num;
    int16_t value;

    for (size_t i = 0; i < ARRAY_SIZE(s_sender_assignment); ++i)
    {
        memset(s_sender_assignment[i].tx_buff, 0, sizeof(s_sender_assignment[i].tx_buff));
    }

    for (size_t i = 0; i < s_dji_motor_count; ++i)
    {
        DJIMotorInstance *motor = s_dji_motor_instance[i];
        Motor_Controller_s *motor_controller = &motor->motor_controller;
        uint8_t freq_idx = (uint8_t)motor_controller->control_freq;
        uint8_t period;

        tick_counter[i]++;
        if (freq_idx >= (uint8_t)(sizeof(s_freq_period_ms) / sizeof(s_freq_period_ms[0])))
        {
            freq_idx = (uint8_t)MOTOR_FREQ_1000HZ;
        }
        period = s_freq_period_ms[freq_idx];

        if (tick_counter[i] >= period)
        {
            tick_counter[i] = 0U;
            motor_controller->control_cache = DJIMotorCompute(motor);
        }

        value = (motor->stop_flag == MOTOR_STOP) ? 0 : motor_controller->control_cache;
        group = motor->sender_group;
        num = motor->message_num;
        s_sender_assignment[group].tx_buff[2U * num] = (uint8_t)(value >> 8);
        s_sender_assignment[group].tx_buff[2U * num + 1U] = (uint8_t)(value & 0xFF);
    }

    for (size_t i = 0; i < ARRAY_SIZE(s_sender_assignment); ++i)
    {
        if (s_sender_enable_flag[i] != 0U)
        {
            (void)CANTransmitTry(&s_sender_assignment[i]);
        }
    }
}

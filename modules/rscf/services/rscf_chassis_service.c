#include <zephyr/logging/log.h>

#include <rscf/rscf_chassis_service.h>

#include "bsp_can.h"
#include "dji_motor.h"
#include "general_def.h"

LOG_MODULE_REGISTER(rscf_chassis_service, LOG_LEVEL_INF);

#define RSCF_CHASSIS_WHEEL_COUNT 4U
#define RSCF_CHASSIS_DJI_HUB_CTRL_FREQ MOTOR_FREQ_500HZ
#define RSCF_WHEEL_RADIUS_MM (105.0f * 0.5f)
#define RSCF_WHEELBASE_MM 500.0f
#define RSCF_WHEELTRACK_MM 500.0f
#define RSCF_WHEEL_RADIUS_M (RSCF_WHEEL_RADIUS_MM * 0.001f)
#define RSCF_WHEEL_CIRCUMFERENCE_M (2.0f * PI * RSCF_WHEEL_RADIUS_M)
#define RSCF_RX_M (RSCF_WHEELTRACK_MM * 0.5f * 0.001f)
#define RSCF_RY_M (RSCF_WHEELBASE_MM * 0.5f * 0.001f)

static DJIMotorInstance *s_chassis_m3508_motor[RSCF_CHASSIS_WHEEL_COUNT];
static const int8_t s_chassis_direction[RSCF_CHASSIS_WHEEL_COUNT] = {1, -1, 1, -1};
static bool s_chassis_initialized;

static float RSCFChassisLegacySpeedPidGainScale(void)
{
    return DJIMotorGetDefaultCurMaPerRaw(M3508) * DJIMotorGetDefaultRatio(M3508) * DEG_PER_RAD;
}

static Motor_Reverse_Flag_e RSCFChassisReverseFlag(int direction_inv)
{
    return (direction_inv < 0) ? MOTOR_DIRECTION_REVERSE : MOTOR_DIRECTION_NORMAL;
}

static void RSCFChassisApplyM3508Pid(Motor_Init_Config_s *cfg)
{
    const float speed_pid_gain_scale = RSCFChassisLegacySpeedPidGainScale();

    if (cfg == NULL)
    {
        return;
    }

    cfg->controller_param_init_config.speed_PID.Kp = 1.8f * speed_pid_gain_scale;
    cfg->controller_param_init_config.speed_PID.Kd = 0.0f;
    cfg->controller_param_init_config.speed_PID.Ki = 0.003f * speed_pid_gain_scale;
    cfg->controller_param_init_config.speed_PID.IntegralLimit = 1200.0f * DJIMotorGetDefaultCurMaPerRaw(M3508);
    cfg->controller_param_init_config.speed_PID.Improve = PID_Integral_Limit;
    cfg->controller_param_init_config.speed_PID.MaxOut = 16000.0f * DJIMotorGetDefaultCurMaPerRaw(M3508);
    cfg->controller_param_init_config.speed_PID.DeadBand =
        UnitRotorToOutputVelRadps(UnitDegToRad(60.0f), DJIMotorGetDefaultRatio(M3508));

    cfg->controller_param_init_config.angle_PID.Kp = 8.0f;
    cfg->controller_param_init_config.angle_PID.Kd = 0.0f;
    cfg->controller_param_init_config.angle_PID.Ki = 0.02f;
    cfg->controller_param_init_config.angle_PID.IntegralLimit =
        UnitRotorToOutputVelRadps(UnitDegToRad(3000.0f), DJIMotorGetDefaultRatio(M3508));
    cfg->controller_param_init_config.angle_PID.Improve = PID_Integral_Limit;
    cfg->controller_param_init_config.angle_PID.MaxOut =
        UnitRotorToOutputVelRadps(UnitDegToRad(36000.0f), DJIMotorGetDefaultRatio(M3508));
    cfg->controller_param_init_config.angle_PID.DeadBand =
        UnitRotorToOutputPosRad(UnitDegToRad(2.0f), DJIMotorGetDefaultRatio(M3508));
}

static float RSCFChassisWheelLinearSpeedToMotorRpm(float wheel_linear_mps)
{
    if (RSCF_WHEEL_CIRCUMFERENCE_M <= 0.0f)
    {
        return 0.0f;
    }

    return wheel_linear_mps / RSCF_WHEEL_CIRCUMFERENCE_M * 60.0f;
}

static void RSCFChassisInverseKinematics(float vx, float vy, float yaw_rate_degps, float out_wheel_mps[4])
{
    float w_term = UnitDegToRad(yaw_rate_degps) * (RSCF_RX_M + RSCF_RY_M);

    out_wheel_mps[0] = vx - vy - w_term;
    out_wheel_mps[1] = vx + vy + w_term;
    out_wheel_mps[2] = vx + vy - w_term;
    out_wheel_mps[3] = vx - vy + w_term;
}

int RSCFChassisServiceInit(void)
{
    uint8_t i;

    if (s_chassis_initialized)
    {
        return 0;
    }

    for (i = 0U; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
    {
        Motor_Init_Config_s cfg = {0};

        cfg.motor_type = M3508;
        cfg.controller_setting_init_config.outer_loop_type = SPEED_LOOP;
        cfg.controller_setting_init_config.close_loop_type = SPEED_LOOP;
        cfg.controller_setting_init_config.motor_reverse_flag = RSCFChassisReverseFlag(s_chassis_direction[i]);
        cfg.controller_setting_init_config.feedback_reverse_flag = FEEDBACK_DIRECTION_NORMAL;
        cfg.controller_setting_init_config.angle_feedback_source = MOTOR_FEED;
        cfg.controller_setting_init_config.speed_feedback_source = MOTOR_FEED;
        cfg.controller_setting_init_config.feedforward_flag = FEEDFORWARD_NONE;
        cfg.can_init_config.can_handle = &hcan1;
        cfg.can_init_config.tx_id = (uint32_t)(i + 1U);

        RSCFChassisApplyM3508Pid(&cfg);
        s_chassis_m3508_motor[i] = DJIMotorInit(&cfg);
        if (s_chassis_m3508_motor[i] != NULL)
        {
            DJIMotorSetFreq(s_chassis_m3508_motor[i], RSCF_CHASSIS_DJI_HUB_CTRL_FREQ);
            DJIMotorEnable(s_chassis_m3508_motor[i]);
            DJIMotorSetRef(s_chassis_m3508_motor[i], 0.0f);
        }
    }

    s_chassis_initialized = true;
    LOG_INF("chassis service initialized");
    return 0;
}

void RSCFChassisServiceSetEnable(bool enable)
{
    for (size_t i = 0; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
    {
        if (s_chassis_m3508_motor[i] == NULL)
        {
            continue;
        }

        if (enable)
        {
            DJIMotorEnable(s_chassis_m3508_motor[i]);
        }
        else
        {
            DJIMotorStop(s_chassis_m3508_motor[i]);
        }
    }
}

void RSCFChassisServiceStop(void)
{
    for (size_t i = 0; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
    {
        if (s_chassis_m3508_motor[i] != NULL)
        {
            DJIMotorSetRef(s_chassis_m3508_motor[i], 0.0f);
        }
    }
}

void RSCFChassisServiceSetTarget(float vx_mps, float vy_mps, float yaw_rate_degps)
{
    float wheel_linear_mps[RSCF_CHASSIS_WHEEL_COUNT] = {0.0f};

    RSCFChassisInverseKinematics(vx_mps, vy_mps, yaw_rate_degps, wheel_linear_mps);
    for (size_t i = 0; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
    {
        float target_rpm = RSCFChassisWheelLinearSpeedToMotorRpm(wheel_linear_mps[i]);

        if (s_chassis_m3508_motor[i] != NULL)
        {
            DJIMotorSetRef(s_chassis_m3508_motor[i], UnitRpmToRadps(target_rpm));
        }
    }
}

bool RSCFChassisServiceAnyMotorOnline(void)
{
    for (size_t i = 0; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
    {
        if ((s_chassis_m3508_motor[i] != NULL) && (s_chassis_m3508_motor[i]->daemon != NULL) &&
            (DaemonIsOnline(s_chassis_m3508_motor[i]->daemon) != 0U))
        {
            return true;
        }
    }

    return false;
}

bool RSCFChassisServiceAllMotorsOnline(void)
{
    uint8_t configured_count = 0U;
    uint8_t online_count = 0U;

    for (size_t i = 0; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
    {
        if (s_chassis_m3508_motor[i] != NULL)
        {
            configured_count++;
            if ((s_chassis_m3508_motor[i]->daemon != NULL) &&
                (DaemonIsOnline(s_chassis_m3508_motor[i]->daemon) != 0U))
            {
                online_count++;
            }
        }
    }

    return (configured_count != 0U) && (configured_count == online_count);
}

#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/reboot.h>

#include <rscf/rscf_chassis_service.h>

#include "bsp_can.h"
#include "comm_manager.h"
#include "controller.h"
#include "data_def.h"
#include "dji_motor.h"
#include "general_def.h"
#include "robot_cmd.h"
#include "robot_interface.h"
#include "rscf_imu_uart.h"
#include "unit_convert.h"

LOG_MODULE_REGISTER(rscf_chassis_service, LOG_LEVEL_INF);

#define RSCF_CHASSIS_WHEEL_COUNT 4U
#define RSCF_CHASSIS_DJI_HUB_CTRL_FREQ MOTOR_FREQ_500HZ
#define RSCF_CHASSIS_CTRL_PERIOD_MS 5U
#define RSCF_CHASSIS_ODOM_PERIOD_MS 2U
#define RSCF_CHASSIS_STATUS_PERIOD_MS 10U
#define RSCF_WHEEL_RADIUS_MM (105.0f * 0.5f)
#define RSCF_WHEELBASE_MM 500.0f
#define RSCF_WHEELTRACK_MM 500.0f
#define RSCF_WHEEL_RADIUS_M (RSCF_WHEEL_RADIUS_MM * 0.001f)
#define RSCF_WHEEL_CIRCUMFERENCE_M (2.0f * PI * RSCF_WHEEL_RADIUS_M)
#define RSCF_RX_M (RSCF_WHEELTRACK_MM * 0.5f * 0.001f)
#define RSCF_RY_M (RSCF_WHEELBASE_MM * 0.5f * 0.001f)
#define RSCF_MAX_LINEAR_VEL_MPS 5.0f
#define RSCF_MAX_YAW_RATE_DEGPS 180.0f
#define RSCF_YAW_RATE_EPS_DEG 0.01f
#define RSCF_SWITCH_RESET_MASK 0x0001U
#define RSCF_SWITCH_FRAME_BODY_MASK 0x0002U

typedef struct
{
    float vx_mps;
    float vy_mps;
    float wz_radps;
    float delta_x_m;
    float delta_y_m;
    float delta_yaw_rad;
} RSCFChassisBodyOdom_t;

typedef struct
{
    float x_m;
    float y_m;
    float yaw_rad;
    float vx_mps;
    float vy_mps;
    float wz_radps;
} RSCFChassisMapOdom_t;

typedef struct
{
    bool initialized;
    bool odom_ok;
    bool imu_ready;
    bool yaw_initialized;
    bool odom_seeded;
    bool map_yaw_seeded;
    bool heading_pid_initialized;
    uint8_t ctrl_mode;
    uint8_t velocity_frame;
    uint8_t heading_mode;
    uint8_t yaw_mode;
    uint8_t yaw_lock_enable;
    float target_speed_mps;
    float move_direction_rad;
    float angular_velocity_degps;
    float heading_target_deg;
    float linear_x_mps;
    float linear_y_mps;
    float yaw_rate_degps;
    float yaw_target_deg;
    float map_yaw_origin_deg;
    float imu_roll_deg;
    float imu_pitch_deg;
    float imu_yaw_deg;
    float imu_gyro_z_dps;
    float imu_acc_x;
    float imu_acc_y;
    float imu_acc_z;
    float last_raw_yaw_deg;
    float last_odom_m[RSCF_CHASSIS_WHEEL_COUNT];
    PIDInstance heading_pid;
    RSCFChassisBodyOdom_t body_odom;
    RSCFChassisMapOdom_t map_odom;
} RSCFChassisRuntime_s;

static DJIMotorInstance *s_chassis_m3508_motor[RSCF_CHASSIS_WHEEL_COUNT];
static const int8_t s_chassis_direction[RSCF_CHASSIS_WHEEL_COUNT] = {1, -1, 1, -1};
static RSCFChassisRuntime_s s_chassis;

static float RSCFChassisLegacySpeedPidGainScale(void)
{
    return DJIMotorGetDefaultCurMaPerRaw(M3508) * DJIMotorGetDefaultRatio(M3508) * DEG_PER_RAD;
}

static float RSCFChassisAbsF(float value)
{
    return (value >= 0.0f) ? value : -value;
}

static float RSCFChassisWrapRad(float angle_rad)
{
    while (angle_rad > PI)
    {
        angle_rad -= (2.0f * PI);
    }

    while (angle_rad <= -PI)
    {
        angle_rad += (2.0f * PI);
    }

    return angle_rad;
}

static float RSCFChassisFastSin(float angle_rad)
{
    float y;

    angle_rad = RSCFChassisWrapRad(angle_rad);
    if (angle_rad < 0.0f)
    {
        y = 1.27323954f * angle_rad + 0.405284735f * angle_rad * angle_rad;
    }
    else
    {
        y = 1.27323954f * angle_rad - 0.405284735f * angle_rad * angle_rad;
    }

    if (y < 0.0f)
    {
        return 0.225f * (y * -y - y) + y;
    }

    return 0.225f * (y * y - y) + y;
}

static float RSCFChassisFastCos(float angle_rad)
{
    return RSCFChassisFastSin(angle_rad + (0.5f * PI));
}

static float RSCFChassisFastAtan2(float y, float x)
{
    float abs_y = RSCFChassisAbsF(y) + 1e-10f;
    float angle;

    if (x >= 0.0f)
    {
        float r = (x - abs_y) / (x + abs_y);

        angle = (0.25f * PI) - (0.25f * PI) * r;
    }
    else
    {
        float r = (x + abs_y) / (abs_y - x);

        angle = (0.75f * PI) - (0.25f * PI) * r;
    }

    return (y < 0.0f) ? -angle : angle;
}

static float RSCFChassisFastSqrt(float value)
{
    float guess;

    if (value <= 0.0f)
    {
        return 0.0f;
    }

    guess = (value > 1.0f) ? value : 1.0f;
    for (uint8_t i = 0U; i < 6U; ++i)
    {
        guess = 0.5f * (guess + value / guess);
    }

    return guess;
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

static void RSCFChassisRegisterM3508Motors(void)
{
    for (size_t i = 0U; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
    {
        Motor_Init_Config_s cfg = {0};

        if (s_chassis_m3508_motor[i] != NULL)
        {
            continue;
        }

        cfg.motor_type = M3508;
        cfg.controller_setting_init_config.outer_loop_type = SPEED_LOOP;
        cfg.controller_setting_init_config.close_loop_type = SPEED_LOOP;
        cfg.controller_setting_init_config.motor_reverse_flag =
            RSCFChassisReverseFlag(s_chassis_direction[i]);
        cfg.controller_setting_init_config.feedback_reverse_flag = FEEDBACK_DIRECTION_NORMAL;
        cfg.controller_setting_init_config.angle_feedback_source = MOTOR_FEED;
        cfg.controller_setting_init_config.speed_feedback_source = MOTOR_FEED;
        cfg.controller_setting_init_config.feedforward_flag = FEEDFORWARD_NONE;
        cfg.can_init_config.can_handle = &hcan1;
        cfg.can_init_config.tx_id = (uint32_t)(i + 1U);

        RSCFChassisApplyM3508Pid(&cfg);
        s_chassis_m3508_motor[i] = DJIMotorInit(&cfg);
        if (s_chassis_m3508_motor[i] == NULL)
        {
            LOG_WRN("chassis motor %u register failed", (unsigned int)i);
            continue;
        }

        DJIMotorSetFreq(s_chassis_m3508_motor[i], RSCF_CHASSIS_DJI_HUB_CTRL_FREQ);
    }
}

static void RSCFChassisSetBits(uint8_t *bytes, uint16_t bit_offset, uint8_t bit_width, uint8_t value)
{
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

static void RSCFChassisSetStatusField(uint16_t bit_offset, uint8_t bit_width, uint8_t value)
{
    RSCFChassisSetBits(g_system_status.status_bytes, bit_offset, bit_width, value);
}

static void RSCFChassisSetServiceReply(uint8_t service_id, uint8_t service_result, uint8_t service_ticket)
{
    RSCFChassisSetStatusField(SYSTEM_STATUS_SERVICE_ID_OFFSET, SYSTEM_STATUS_SERVICE_ID_WIDTH, service_id);
    RSCFChassisSetStatusField(SYSTEM_STATUS_SERVICE_RESULT_OFFSET, SYSTEM_STATUS_SERVICE_RESULT_WIDTH, service_result);
    RSCFChassisSetStatusField(SYSTEM_STATUS_SERVICE_TICKET_OFFSET, SYSTEM_STATUS_SERVICE_TICKET_WIDTH, service_ticket);
}

static void RSCFChassisSetActionReply(uint8_t action_id,
                                      uint8_t action_state,
                                      uint8_t action_ticket,
                                      uint8_t action_result,
                                      uint8_t action_progress)
{
    RSCFChassisSetStatusField(SYSTEM_STATUS_ACTION_ID_OFFSET, SYSTEM_STATUS_ACTION_ID_WIDTH, action_id);
    RSCFChassisSetStatusField(SYSTEM_STATUS_ACTION_STATE_OFFSET, SYSTEM_STATUS_ACTION_STATE_WIDTH, action_state);
    RSCFChassisSetStatusField(SYSTEM_STATUS_ACTION_TICKET_OFFSET, SYSTEM_STATUS_ACTION_TICKET_WIDTH, action_ticket);
    RSCFChassisSetStatusField(SYSTEM_STATUS_ACTION_RESULT_OFFSET, SYSTEM_STATUS_ACTION_RESULT_WIDTH, action_result);
    RSCFChassisSetStatusField(SYSTEM_STATUS_ACTION_PROGRESS_OFFSET, SYSTEM_STATUS_ACTION_PROGRESS_WIDTH, action_progress);
}

static void RSCFChassisRefreshBaseStatus(void)
{
    uint8_t imu_state = CONTROL_PLANE_IMU_STATE_DISABLED;

    if (RSCFImuUartReady())
    {
        imu_state = s_chassis.imu_ready ? CONTROL_PLANE_IMU_STATE_OK : CONTROL_PLANE_IMU_STATE_FAULT;
    }

    RSCFChassisSetStatusField(SYSTEM_STATUS_INIT_DONE_OFFSET, SYSTEM_STATUS_INIT_DONE_WIDTH, 1U);
    RSCFChassisSetStatusField(SYSTEM_STATUS_ODOM_OK_OFFSET, SYSTEM_STATUS_ODOM_OK_WIDTH, s_chassis.odom_ok ? 1U : 0U);
    RSCFChassisSetStatusField(SYSTEM_STATUS_TASK_ALL_OK_OFFSET,
                              SYSTEM_STATUS_TASK_ALL_OK_WIDTH,
                              RSCFChassisServiceAllMotorsOnline() ? 1U : 0U);
    RSCFChassisSetStatusField(SYSTEM_STATUS_PEER_MCU_COMM_OK_OFFSET, SYSTEM_STATUS_PEER_MCU_COMM_OK_WIDTH, 0U);
    RSCFChassisSetStatusField(SYSTEM_STATUS_IMU_STATE_OFFSET, SYSTEM_STATUS_IMU_STATE_WIDTH, imu_state);
    RSCFChassisSetStatusField(SYSTEM_STATUS_CTRL_MODE_OFFSET, SYSTEM_STATUS_CTRL_MODE_WIDTH, s_chassis.ctrl_mode);
    RSCFChassisSetStatusField(SYSTEM_STATUS_HEADING_MODE_OFFSET, SYSTEM_STATUS_HEADING_MODE_WIDTH, s_chassis.heading_mode);
    RSCFChassisSetStatusField(SYSTEM_STATUS_VELOCITY_FRAME_OFFSET,
                              SYSTEM_STATUS_VELOCITY_FRAME_WIDTH,
                              s_chassis.velocity_frame);
}

static void RSCFChassisRequestStatusTx(void)
{
    RobotInterfaceRequestTxEvent(ROBOT_INTERFACE_TX_EVENT_SYSTEM_STATUS);
}

static bool RSCFChassisCtrlModeAllowsMotion(uint8_t ctrl_mode)
{
    return (ctrl_mode == CONTROL_PLANE_CHASSIS_CTRL_MODE_MANUAL) ||
           (ctrl_mode == CONTROL_PLANE_CHASSIS_CTRL_MODE_CMD_VEL);
}

static void RSCFChassisResetHeadingPid(void)
{
    memset(&s_chassis.heading_pid, 0, sizeof(s_chassis.heading_pid));
    s_chassis.heading_pid_initialized = false;
}

static void RSCFChassisApplyDefaults(void)
{
    memset(&s_chassis, 0, sizeof(s_chassis));
    s_chassis.ctrl_mode = CONTROL_PLANE_CHASSIS_CTRL_MODE_CMD_VEL;
    s_chassis.velocity_frame = CONTROL_PLANE_VELOCITY_FRAME_WORLD;
    s_chassis.heading_mode = CONTROL_PLANE_HEADING_MODE_LOCK_ZERO;
    s_chassis.yaw_lock_enable = 1U;
    s_chassis.yaw_mode = 0U;
    s_chassis.target_speed_mps = 0.0f;
    s_chassis.move_direction_rad = 0.0f;
    s_chassis.angular_velocity_degps = 0.0f;
    s_chassis.heading_target_deg = 0.0f;
    RSCFChassisResetHeadingPid();
}

static void RSCFChassisClearCommandAndOutput(void)
{
    s_chassis.target_speed_mps = 0.0f;
    s_chassis.move_direction_rad = 0.0f;
    s_chassis.angular_velocity_degps = 0.0f;
    s_chassis.linear_x_mps = 0.0f;
    s_chassis.linear_y_mps = 0.0f;
    s_chassis.yaw_rate_degps = 0.0f;
    s_chassis.yaw_target_deg = s_chassis.imu_yaw_deg;
}

static float RSCFChassisNormalizeAngleDeg(float angle_deg)
{
    while (angle_deg > 180.0f)
    {
        angle_deg -= 360.0f;
    }

    while (angle_deg <= -180.0f)
    {
        angle_deg += 360.0f;
    }

    return angle_deg;
}

static float RSCFChassisSelectNearestHeadingDeg(float current_yaw_deg, float target_yaw_deg)
{
    float delta_deg = RSCFChassisNormalizeAngleDeg(target_yaw_deg - current_yaw_deg);

    return current_yaw_deg + delta_deg;
}

static void RSCFChassisEnsureHeadingPidInitialized(void)
{
    if (s_chassis.heading_pid_initialized)
    {
        return;
    }

    PID_Init_Config_s cfg = {
        .Kp = 10.0f,
        .Ki = 0.01f,
        .Kd = 0.1f,
        .MaxOut = RSCF_MAX_YAW_RATE_DEGPS,
        .DeadBand = RSCF_YAW_RATE_EPS_DEG,
        .Improve = PID_Integral_Limit,
        .IntegralLimit = 60.0f,
    };

    PIDInit(&s_chassis.heading_pid, &cfg);
    s_chassis.heading_pid_initialized = true;
}

static float RSCFChassisWheelLinearSpeedToMotorRpm(float wheel_linear_mps)
{
    if (RSCF_WHEEL_CIRCUMFERENCE_M <= 0.0f)
    {
        return 0.0f;
    }

    return wheel_linear_mps / RSCF_WHEEL_CIRCUMFERENCE_M * 60.0f;
}

static void RSCFChassisForwardKinematics(float v_fl,
                                         float v_fr,
                                         float v_rl,
                                         float v_rr,
                                         float *out_vx,
                                         float *out_vy)
{
    if (out_vx != NULL)
    {
        *out_vx = (v_fl + v_fr + v_rl + v_rr) * 0.25f;
    }

    if (out_vy != NULL)
    {
        *out_vy = (-v_fl + v_fr + v_rl - v_rr) * 0.25f;
    }
}

static void RSCFChassisInverseKinematics(float vx, float vy, float yaw_rate_degps, float out_wheel_mps[4])
{
    float w_term = UnitDegToRad(yaw_rate_degps) * (RSCF_RX_M + RSCF_RY_M);

    out_wheel_mps[0] = vx - vy - w_term;
    out_wheel_mps[1] = vx + vy + w_term;
    out_wheel_mps[2] = vx + vy - w_term;
    out_wheel_mps[3] = vx - vy + w_term;
}

static void RSCFChassisSyncImuState(void)
{
    struct rscf_saber_topic_data latest_frame = {0};

    if (!RSCFImuUartTryGetLatest(&latest_frame))
    {
        if (!RSCFImuUartReady())
        {
            s_chassis.imu_ready = false;
        }
        return;
    }

    s_chassis.imu_acc_x = latest_frame.kalman_acc.x;
    s_chassis.imu_acc_y = latest_frame.kalman_acc.y;
    s_chassis.imu_acc_z = latest_frame.kalman_acc.z;
    s_chassis.imu_gyro_z_dps = latest_frame.kalman_gyro.z;
    s_chassis.imu_roll_deg = latest_frame.euler.roll;
    s_chassis.imu_pitch_deg = latest_frame.euler.pitch;

    if (!s_chassis.yaw_initialized)
    {
        s_chassis.last_raw_yaw_deg = latest_frame.euler.yaw;
        s_chassis.imu_yaw_deg = latest_frame.euler.yaw;
        s_chassis.yaw_initialized = true;
    }
    else
    {
        float delta_yaw_deg = latest_frame.euler.yaw - s_chassis.last_raw_yaw_deg;

        if (delta_yaw_deg > 180.0f)
        {
            delta_yaw_deg -= 360.0f;
        }
        else if (delta_yaw_deg < -180.0f)
        {
            delta_yaw_deg += 360.0f;
        }

        s_chassis.imu_yaw_deg += delta_yaw_deg;
        s_chassis.last_raw_yaw_deg = latest_frame.euler.yaw;
    }

    if (!s_chassis.map_yaw_seeded)
    {
        s_chassis.map_yaw_origin_deg = s_chassis.imu_yaw_deg;
        s_chassis.map_yaw_seeded = true;
    }

    s_chassis.imu_ready = true;
}

static void RSCFChassisUpdateWheelFeedback(float wheel_speed_mps[4], float wheel_odom_m[4])
{
    for (size_t i = 0U; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
    {
        wheel_speed_mps[i] = 0.0f;
        wheel_odom_m[i] = 0.0f;

        if (s_chassis_m3508_motor[i] == NULL)
        {
            continue;
        }

        wheel_speed_mps[i] =
            UnitOutputToWheelMps(s_chassis_m3508_motor[i]->fb.out.vel_radps, RSCF_WHEEL_RADIUS_M) *
            (float)s_chassis_direction[i];
        wheel_odom_m[i] = s_chassis_m3508_motor[i]->fb.out.pos_rad * RSCF_WHEEL_RADIUS_M * (float)s_chassis_direction[i];
    }
}

static void RSCFChassisUpdateBodyOdom(void)
{
    float wheel_speed_mps[4] = {0.0f};
    float wheel_odom_m[4] = {0.0f};
    float delta_m[4] = {0.0f};
    float body_dx = 0.0f;
    float body_dy = 0.0f;
    float body_vx = 0.0f;
    float body_vy = 0.0f;

    RSCFChassisUpdateWheelFeedback(wheel_speed_mps, wheel_odom_m);

    if (!s_chassis.odom_seeded)
    {
        memcpy(s_chassis.last_odom_m, wheel_odom_m, sizeof(s_chassis.last_odom_m));
        s_chassis.odom_seeded = true;
    }

    for (size_t i = 0U; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
    {
        delta_m[i] = wheel_odom_m[i] - s_chassis.last_odom_m[i];
        s_chassis.last_odom_m[i] = wheel_odom_m[i];
    }

    RSCFChassisForwardKinematics(delta_m[0], delta_m[1], delta_m[2], delta_m[3], &body_dx, &body_dy);
    RSCFChassisForwardKinematics(
        wheel_speed_mps[0], wheel_speed_mps[1], wheel_speed_mps[2], wheel_speed_mps[3], &body_vx, &body_vy);

    s_chassis.body_odom.vx_mps = body_vx;
    s_chassis.body_odom.vy_mps = body_vy;
    s_chassis.body_odom.wz_radps = UnitDpsToRadps(s_chassis.imu_gyro_z_dps);
    s_chassis.body_odom.delta_x_m = body_dx;
    s_chassis.body_odom.delta_y_m = body_dy;
    s_chassis.body_odom.delta_yaw_rad = 0.0f;
    s_chassis.odom_ok = true;
}

static void RSCFChassisUpdateMapOdom(void)
{
    float yaw_rad = s_chassis.map_odom.yaw_rad;

    if (s_chassis.map_yaw_seeded)
    {
        yaw_rad = UnitDegToRad(s_chassis.imu_yaw_deg - s_chassis.map_yaw_origin_deg);
    }

    s_chassis.map_odom.x_m += s_chassis.body_odom.delta_x_m * RSCFChassisFastCos(yaw_rad) -
                              s_chassis.body_odom.delta_y_m * RSCFChassisFastSin(yaw_rad);
    s_chassis.map_odom.y_m += s_chassis.body_odom.delta_x_m * RSCFChassisFastSin(yaw_rad) +
                              s_chassis.body_odom.delta_y_m * RSCFChassisFastCos(yaw_rad);
    s_chassis.map_odom.yaw_rad = yaw_rad;
    s_chassis.map_odom.vx_mps = s_chassis.body_odom.vx_mps * RSCFChassisFastCos(yaw_rad) -
                                s_chassis.body_odom.vy_mps * RSCFChassisFastSin(yaw_rad);
    s_chassis.map_odom.vy_mps = s_chassis.body_odom.vx_mps * RSCFChassisFastSin(yaw_rad) +
                                s_chassis.body_odom.vy_mps * RSCFChassisFastCos(yaw_rad);
    s_chassis.map_odom.wz_radps = UnitDpsToRadps(s_chassis.imu_gyro_z_dps);
}

static void RSCFChassisResolveVelocityCommand(void)
{
    float speed_mps = s_chassis.target_speed_mps;
    float body_dir_rad = s_chassis.move_direction_rad;

    if ((s_chassis.velocity_frame == CONTROL_PLANE_VELOCITY_FRAME_WORLD) && s_chassis.map_yaw_seeded)
    {
        body_dir_rad -= UnitDegToRad(s_chassis.imu_yaw_deg - s_chassis.map_yaw_origin_deg);
    }

    s_chassis.linear_x_mps = speed_mps * RSCFChassisFastCos(body_dir_rad);
    s_chassis.linear_y_mps = speed_mps * RSCFChassisFastSin(body_dir_rad);
}

static float RSCFChassisResolveYawOutputDegps(void)
{
    float yaw_rate_degps = CLAMP(s_chassis.angular_velocity_degps, -RSCF_MAX_YAW_RATE_DEGPS, RSCF_MAX_YAW_RATE_DEGPS);
    float current_yaw_deg = s_chassis.imu_yaw_deg;
    float target_yaw_deg = current_yaw_deg;

    if (!s_chassis.imu_ready)
    {
        s_chassis.yaw_mode = 2U;
        s_chassis.yaw_target_deg = current_yaw_deg;
        s_chassis.yaw_rate_degps = yaw_rate_degps;
        return yaw_rate_degps;
    }

    if ((yaw_rate_degps >= RSCF_YAW_RATE_EPS_DEG) || (yaw_rate_degps <= -RSCF_YAW_RATE_EPS_DEG))
    {
        s_chassis.yaw_mode = 1U;
        s_chassis.yaw_target_deg = current_yaw_deg;
        s_chassis.yaw_rate_degps = yaw_rate_degps;
        return yaw_rate_degps;
    }

    if ((s_chassis.yaw_lock_enable == 0U) || (s_chassis.heading_mode == CONTROL_PLANE_HEADING_MODE_DISABLED))
    {
        s_chassis.yaw_mode = 2U;
        s_chassis.yaw_target_deg = current_yaw_deg;
        s_chassis.yaw_rate_degps = 0.0f;
        return 0.0f;
    }

    if (s_chassis.heading_mode == CONTROL_PLANE_HEADING_MODE_LOCK_ZERO)
    {
        target_yaw_deg = RSCFChassisSelectNearestHeadingDeg(current_yaw_deg, s_chassis.map_yaw_origin_deg);
    }
    else if ((s_chassis.heading_mode == CONTROL_PLANE_HEADING_MODE_TARGET_HEADING) ||
             (s_chassis.heading_mode == CONTROL_PLANE_HEADING_MODE_FOLLOW_TARGET))
    {
        target_yaw_deg = RSCFChassisSelectNearestHeadingDeg(current_yaw_deg, s_chassis.heading_target_deg);
    }

    RSCFChassisEnsureHeadingPidInitialized();
    s_chassis.yaw_mode = 0U;
    s_chassis.yaw_target_deg = target_yaw_deg;
    s_chassis.yaw_rate_degps =
        CLAMP(PIDCalculate(&s_chassis.heading_pid, current_yaw_deg, target_yaw_deg),
              -RSCF_MAX_YAW_RATE_DEGPS,
              RSCF_MAX_YAW_RATE_DEGPS);
    return s_chassis.yaw_rate_degps;
}

static void RSCFChassisApplyWheelTargets(float vx_mps, float vy_mps, float yaw_rate_degps)
{
    float wheel_linear_mps[4] = {0.0f};

    RSCFChassisInverseKinematics(vx_mps, vy_mps, yaw_rate_degps, wheel_linear_mps);
    for (size_t i = 0U; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
    {
        if (s_chassis_m3508_motor[i] != NULL)
        {
            DJIMotorSetRef(s_chassis_m3508_motor[i],
                           UnitRpmToRadps(RSCFChassisWheelLinearSpeedToMotorRpm(wheel_linear_mps[i])));
        }
    }
}

static void RSCFChassisControlStep(void)
{
    float yaw_output_degps = 0.0f;

    RSCFChassisSyncImuState();

    if (!RSCFChassisCtrlModeAllowsMotion(s_chassis.ctrl_mode))
    {
        RSCFChassisClearCommandAndOutput();

        if (s_chassis.ctrl_mode == CONTROL_PLANE_CHASSIS_CTRL_MODE_RELAX)
        {
            RSCFChassisServiceStop();
            RSCFChassisServiceSetEnable(false);
        }
        else
        {
            RSCFChassisServiceSetEnable(true);
            RSCFChassisServiceStop();
        }
        return;
    }

    RSCFChassisServiceSetEnable(true);
    RSCFChassisResolveVelocityCommand();
    yaw_output_degps = RSCFChassisResolveYawOutputDegps();
    RSCFChassisApplyWheelTargets(s_chassis.linear_x_mps, s_chassis.linear_y_mps, yaw_output_degps);
}

static void RSCFChassisOdomStep(void)
{
    RSCFChassisSyncImuState();
    RSCFChassisUpdateBodyOdom();
    RSCFChassisUpdateMapOdom();
}

int RSCFChassisServiceInit(void)
{
    if (s_chassis.initialized)
    {
        return 0;
    }

    RSCFChassisApplyDefaults();
    memset(&g_system_status, 0, sizeof(g_system_status));
    RSCFChassisRefreshBaseStatus();
    RSCFChassisSetServiceReply(CONTROL_PLANE_SERVICE_ID_NONE, CONTROL_PLANE_SERVICE_RESULT_NONE, 0U);
    RSCFChassisSetActionReply(CONTROL_PLANE_ACTION_ID_NONE,
                              CONTROL_PLANE_ACTION_STATE_IDLE,
                              0U,
                              CONTROL_PLANE_ACTION_RESULT_NONE,
                              0U);
    RSCFChassisRegisterM3508Motors();

    s_chassis.initialized = true;
    LOG_INF("chassis service initialized");
    return 0;
}

void RSCFChassisServiceTick(void)
{
    static uint32_t s_last_ctrl_tick = 0U;
    static uint32_t s_last_odom_tick = 0U;
    uint32_t now_tick;

    if (!s_chassis.initialized)
    {
        return;
    }

    now_tick = k_uptime_get_32();
    if ((uint32_t)(now_tick - s_last_odom_tick) >= RSCF_CHASSIS_ODOM_PERIOD_MS)
    {
        s_last_odom_tick = now_tick;
        RSCFChassisOdomStep();
    }

    if ((uint32_t)(now_tick - s_last_ctrl_tick) >= RSCF_CHASSIS_CTRL_PERIOD_MS)
    {
        s_last_ctrl_tick = now_tick;
        RSCFChassisControlStep();
    }
}

void RSCFChassisServiceSetEnable(bool enable)
{
    for (size_t i = 0U; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
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
    RSCFChassisClearCommandAndOutput();

    for (size_t i = 0U; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
    {
        if (s_chassis_m3508_motor[i] != NULL)
        {
            DJIMotorSetRef(s_chassis_m3508_motor[i], 0.0f);
        }
    }
}

void RSCFChassisServiceSetTarget(float vx_mps, float vy_mps, float yaw_rate_degps)
{
    s_chassis.ctrl_mode = CONTROL_PLANE_CHASSIS_CTRL_MODE_CMD_VEL;
    s_chassis.velocity_frame = CONTROL_PLANE_VELOCITY_FRAME_BODY;
    s_chassis.target_speed_mps =
        CLAMP(RSCFChassisFastSqrt(vx_mps * vx_mps + vy_mps * vy_mps), 0.0f, RSCF_MAX_LINEAR_VEL_MPS);
    s_chassis.move_direction_rad = RSCFChassisFastAtan2(vy_mps, vx_mps);
    s_chassis.angular_velocity_degps = CLAMP(yaw_rate_degps, -RSCF_MAX_YAW_RATE_DEGPS, RSCF_MAX_YAW_RATE_DEGPS);
    RSCFChassisRequestStatusTx();
}

bool RSCFChassisServiceAnyMotorOnline(void)
{
    for (size_t i = 0U; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
    {
        if ((s_chassis_m3508_motor[i] != NULL) && (s_chassis_m3508_motor[i]->daemon != NULL) &&
            (DaemonIsOnline(s_chassis_m3508_motor[i]->daemon) != 0U))
        {
            return true;
        }
    }

    return false;
}

void RSCFChassisServiceSetControlMode(uint8_t ctrl_mode)
{
    s_chassis.ctrl_mode = ctrl_mode;
}

void RSCFChassisServiceSetHeadingMode(uint8_t heading_mode)
{
    s_chassis.heading_mode = heading_mode;
}

void RSCFChassisServiceSetVelocityFrame(uint8_t velocity_frame)
{
    s_chassis.velocity_frame = velocity_frame;
}

void RSCFChassisServiceSetHeadingTarget(float heading_deg)
{
    s_chassis.heading_target_deg = heading_deg;
    RSCFChassisRequestStatusTx();
}

uint8_t RSCFChassisServiceGetControlMode(void)
{
    return s_chassis.ctrl_mode;
}

uint8_t RSCFChassisServiceGetHeadingMode(void)
{
    return s_chassis.heading_mode;
}

uint8_t RSCFChassisServiceGetVelocityFrame(void)
{
    return s_chassis.velocity_frame;
}

bool RSCFChassisServiceGetOdom(float *x_m,
                               float *y_m,
                               float *yaw_rad,
                               float *vx_mps,
                               float *vy_mps,
                               float *wz_radps)
{
    if (x_m != NULL)
    {
        *x_m = s_chassis.map_odom.x_m;
    }

    if (y_m != NULL)
    {
        *y_m = s_chassis.map_odom.y_m;
    }

    if (yaw_rad != NULL)
    {
        *yaw_rad = s_chassis.map_odom.yaw_rad;
    }

    if (vx_mps != NULL)
    {
        *vx_mps = s_chassis.map_odom.vx_mps;
    }

    if (vy_mps != NULL)
    {
        *vy_mps = s_chassis.map_odom.vy_mps;
    }

    if (wz_radps != NULL)
    {
        *wz_radps = s_chassis.body_odom.wz_radps;
    }

    return s_chassis.odom_ok;
}

bool RSCFChassisServiceGetImuData(float *roll_deg,
                                  float *pitch_deg,
                                  float *yaw_deg,
                                  float *gyro_z_dps,
                                  float *acc_x,
                                  float *acc_y,
                                  float *acc_z)
{
    if (roll_deg != NULL)
    {
        *roll_deg = s_chassis.imu_roll_deg;
    }

    if (pitch_deg != NULL)
    {
        *pitch_deg = s_chassis.imu_pitch_deg;
    }

    if (yaw_deg != NULL)
    {
        *yaw_deg = s_chassis.imu_yaw_deg;
    }

    if (gyro_z_dps != NULL)
    {
        *gyro_z_dps = s_chassis.imu_gyro_z_dps;
    }

    if (acc_x != NULL)
    {
        *acc_x = s_chassis.imu_acc_x;
    }

    if (acc_y != NULL)
    {
        *acc_y = s_chassis.imu_acc_y;
    }

    if (acc_z != NULL)
    {
        *acc_z = s_chassis.imu_acc_z;
    }

    return s_chassis.imu_ready;
}

bool RSCFChassisServiceAllMotorsOnline(void)
{
    uint8_t configured_count = 0U;
    uint8_t online_count = 0U;

    for (size_t i = 0U; i < RSCF_CHASSIS_WHEEL_COUNT; ++i)
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

void ChassisCtrlCallback(void *data)
{
    const chassisCtrl_t *ctrl = (const chassisCtrl_t *)data;

    if (ctrl == NULL)
    {
        ctrl = &g_chassis_ctrl;
    }

    s_chassis.target_speed_mps = CLAMP(((float)ctrl->speed_mps) * 0.001f, 0.0f, RSCF_MAX_LINEAR_VEL_MPS);
    s_chassis.move_direction_rad = UnitDegToRad(((float)ctrl->direction_cdeg) * 0.01f);
    s_chassis.angular_velocity_degps =
        CLAMP(UnitRadToDeg(((float)ctrl->yaw_rate_radps) * 0.001f), -RSCF_MAX_YAW_RATE_DEGPS, RSCF_MAX_YAW_RATE_DEGPS);
    s_chassis.heading_target_deg = ((float)ctrl->heading_cdeg) * 0.01f;
}

void RobotSwitchCallback(void *data)
{
    const robotSwitch_t *robot_switch = (const robotSwitch_t *)data;
    uint16_t switch_mask = 0U;

    if (robot_switch == NULL)
    {
        robot_switch = &g_robot_switch;
    }

    switch_mask = robot_switch->switch_mask;
    if ((switch_mask & RSCF_SWITCH_FRAME_BODY_MASK) != 0U)
    {
        s_chassis.velocity_frame = CONTROL_PLANE_VELOCITY_FRAME_BODY;
    }
    else
    {
        s_chassis.velocity_frame = CONTROL_PLANE_VELOCITY_FRAME_WORLD;
    }

    RSCFChassisRequestStatusTx();
    if ((switch_mask & RSCF_SWITCH_RESET_MASK) != 0U)
    {
        sys_reboot(SYS_REBOOT_COLD);
    }
}

void SystemCmdCallback(void *data)
{
    RobotCmdHandleSystemCmd((const systemCmd_t *)data);
}

void RobotInterfaceTxPeriodicHook(void)
{
    Comm_Batch_Begin();
    (void)RobotSendChassisFeedback(
        s_chassis.map_odom.vx_mps,
        s_chassis.map_odom.vy_mps,
        UnitMToMm(s_chassis.map_odom.x_m),
        UnitMToMm(s_chassis.map_odom.y_m));

    if (s_chassis.imu_ready)
    {
        (void)RobotSendImuEuler(s_chassis.imu_roll_deg,
                                s_chassis.imu_pitch_deg,
                                s_chassis.imu_yaw_deg - s_chassis.map_yaw_origin_deg);
    }

    RobotCmdCommPublishTick();
    Comm_Batch_End();
}

void RobotInterfaceTxEventHook(uint32_t tx_events)
{
    if ((tx_events & ROBOT_INTERFACE_TX_EVENT_SYSTEM_STATUS) == 0U)
    {
        return;
    }

    RobotCmdCommPublishNow();
}

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

#include "servo_motor.h"

LOG_MODULE_REGISTER(servo_motor, LOG_LEVEL_INF);

#define RSCF_SERVO_NODE DT_NODELABEL(rscf_servo_pwm)

struct pwm_instance
{
    uint32_t period_ns;
};

struct usart_instance
{
    uint8_t reserved;
};

typedef struct
{
    ServoInstance *servo;
    float current_angle;
    float target_angle;
    uint8_t initialized;
} ServoControlContext_t;

#if DT_NODE_HAS_STATUS(RSCF_SERVO_NODE, okay)
static const struct pwm_dt_spec s_servo_pwm = PWM_DT_SPEC_GET(RSCF_SERVO_NODE);
static const float s_servo_min_angle_deg = (float)DT_PROP(RSCF_SERVO_NODE, min_angle_deg);
static const float s_servo_max_angle_deg = (float)DT_PROP(RSCF_SERVO_NODE, max_angle_deg);
static const float s_servo_default_angle_deg = (float)DT_PROP(RSCF_SERVO_NODE, default_angle_deg);
static const float s_servo_min_pulse_us = (float)DT_PROP(RSCF_SERVO_NODE, min_pulse_us);
static const float s_servo_max_pulse_us = (float)DT_PROP(RSCF_SERVO_NODE, max_pulse_us);
static const float s_servo_step_deg = (float)DT_PROP(RSCF_SERVO_NODE, control_step_deg);
#endif

static ServoInstance *s_servo_motor_instance[SERVO_MOTOR_CNT];
static uint8_t s_servo_idx;
static ServoControlContext_t s_servo_control;

static float ServoClamp(float value, float min_value, float max_value)
{
    if (value < min_value)
    {
        return min_value;
    }
    if (value > max_value)
    {
        return max_value;
    }
    return value;
}

static float ServoAngleToPulseUs(float angle_deg)
{
#if DT_NODE_HAS_STATUS(RSCF_SERVO_NODE, okay)
    float clamped_angle = ServoClamp(angle_deg, s_servo_min_angle_deg, s_servo_max_angle_deg);
    return s_servo_min_pulse_us +
           (clamped_angle - s_servo_min_angle_deg) *
           (s_servo_max_pulse_us - s_servo_min_pulse_us) /
           (s_servo_max_angle_deg - s_servo_min_angle_deg);
#else
    ARG_UNUSED(angle_deg);
    return 0.0f;
#endif
}

ServoInstance *ServoInit(Servo_Init_Config_s *servo_init_config)
{
    ServoInstance *servo;

    if (servo_init_config == NULL)
    {
        return NULL;
    }

    if (servo_init_config->servo_type != PWM_Servo)
    {
        LOG_WRN("bus servo is not supported in current Zephyr port");
        return NULL;
    }

#if DT_NODE_HAS_STATUS(RSCF_SERVO_NODE, okay)
    if (!pwm_is_ready_dt(&s_servo_pwm))
    {
        LOG_WRN("servo pwm is not ready");
        return NULL;
    }
#else
    LOG_WRN("servo pwm node is disabled");
    return NULL;
#endif

    servo = (ServoInstance *)malloc(sizeof(ServoInstance));
    if (servo == NULL)
    {
        return NULL;
    }

    memset(servo, 0, sizeof(ServoInstance));
    servo->servo_id = servo_init_config->servo_id;
    servo->servo_type = PWM_Servo;
    servo->angle = s_servo_default_angle_deg;
    s_servo_motor_instance[s_servo_idx++] = servo;
    return servo;
}

void ServoSetAngle(ServoInstance *servo, float angle)
{
    uint32_t pulse_ns;
    uint32_t period_ns;

    if (servo == NULL)
    {
        return;
    }

#if DT_NODE_HAS_STATUS(RSCF_SERVO_NODE, okay)
    servo->angle = ServoClamp(angle, s_servo_min_angle_deg, s_servo_max_angle_deg);
    pulse_ns = (uint32_t)(ServoAngleToPulseUs(servo->angle) * 1000.0f);
    period_ns = (s_servo_pwm.period > 0U) ? s_servo_pwm.period : (uint32_t)(SERVO_PWM_PERIOD_300HZ_S * 1000000000.0f);
    (void)pwm_set_dt(&s_servo_pwm, period_ns, pulse_ns);
#else
    ARG_UNUSED(angle);
#endif
}

uint8_t ServoControlInit(void)
{
    Servo_Init_Config_s servo_init_config;

    if (s_servo_control.initialized != 0U)
    {
        return 1U;
    }

    memset(&servo_init_config, 0, sizeof(servo_init_config));
    servo_init_config.servo_type = PWM_Servo;
    servo_init_config.servo_id = (uint8_t)(SERVO_MOTOR_CNT - 1U);
    servo_init_config.pwm_init_config.period = SERVO_PWM_PERIOD_300HZ_S;
    s_servo_control.servo = ServoInit(&servo_init_config);
    if (s_servo_control.servo == NULL)
    {
        return 0U;
    }

#if DT_NODE_HAS_STATUS(RSCF_SERVO_NODE, okay)
    s_servo_control.current_angle = s_servo_default_angle_deg;
    s_servo_control.target_angle = s_servo_default_angle_deg;
#else
    s_servo_control.current_angle = SERVO_PWM_DEFAULT_ANGLE_DEG;
    s_servo_control.target_angle = SERVO_PWM_DEFAULT_ANGLE_DEG;
#endif
    s_servo_control.initialized = 1U;
    ServoSetAngle(s_servo_control.servo, s_servo_control.current_angle);
    return 1U;
}

void ServoControlTask(void)
{
    float next_angle;
    float step_deg = SERVO_PWM_CONTROL_STEP_DEG;

    if ((s_servo_control.initialized == 0U) || (s_servo_control.servo == NULL))
    {
        return;
    }

#if DT_NODE_HAS_STATUS(RSCF_SERVO_NODE, okay)
    step_deg = s_servo_step_deg;
#endif

    next_angle = s_servo_control.current_angle;
    if (s_servo_control.target_angle > (s_servo_control.current_angle + step_deg))
    {
        next_angle += step_deg;
    }
    else if (s_servo_control.target_angle < (s_servo_control.current_angle - step_deg))
    {
        next_angle -= step_deg;
    }
    else
    {
        next_angle = s_servo_control.target_angle;
    }

#if DT_NODE_HAS_STATUS(RSCF_SERVO_NODE, okay)
    next_angle = ServoClamp(next_angle, s_servo_min_angle_deg, s_servo_max_angle_deg);
#else
    next_angle = ServoClamp(next_angle, SERVO_PWM_ANGLE_MIN_DEG, SERVO_PWM_ANGLE_MAX_DEG);
#endif
    if (next_angle == s_servo_control.current_angle)
    {
        return;
    }

    s_servo_control.current_angle = next_angle;
    ServoSetAngle(s_servo_control.servo, s_servo_control.current_angle);
}

void ServoControlSetTargetAngle(float angle)
{
#if DT_NODE_HAS_STATUS(RSCF_SERVO_NODE, okay)
    s_servo_control.target_angle = ServoClamp(angle, s_servo_min_angle_deg, s_servo_max_angle_deg);
#else
    s_servo_control.target_angle = ServoClamp(angle, SERVO_PWM_ANGLE_MIN_DEG, SERVO_PWM_ANGLE_MAX_DEG);
#endif
}

float ServoControlGetTargetAngle(void)
{
    return s_servo_control.target_angle;
}

float ServoControlGetCurrentAngle(void)
{
    return s_servo_control.current_angle;
}

uint8_t ServoControlIsReady(void)
{
    return s_servo_control.initialized;
}

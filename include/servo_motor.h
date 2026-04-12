#ifndef SERVO_MOTOR_H
#define SERVO_MOTOR_H

#include <stdint.h>

#define SERVO_MOTOR_CNT 7
#define SERVO_PWM_ANGLE_MIN_DEG 0.0f
#define SERVO_PWM_ANGLE_MAX_DEG 180.0f
#define SERVO_PWM_ANGLE_NEUTRAL_DEG 90.0f
#define SERVO_PWM_PULSE_MIN_US 500.0f
#define SERVO_PWM_PULSE_MAX_US 2500.0f
#define SERVO_PWM_PERIOD_300HZ_S (1.0f / 300.0f)
#define SERVO_PWM_PERIOD_200HZ_S (1.0f / 200.0f)
#define SERVO_PWM_DEFAULT_ANGLE_DEG 10.0f
#define SERVO_PWM_CONTROL_STEP_DEG 1.0f
#define SERVO_PWM_CONTROL_TASK_PERIOD_MS 10U

#ifndef RSCF_PWM_INSTANCE_FWD_DECLARED
#define RSCF_PWM_INSTANCE_FWD_DECLARED
typedef struct pwm_instance PWMInstance;
#endif
typedef struct usart_instance USARTInstance;

#ifndef RSCF_PWM_INIT_CONFIG_S_DEFINED
#define RSCF_PWM_INIT_CONFIG_S_DEFINED
typedef struct
{
    void *htim;
    uint32_t channel;
    float period;
    float dutyratio;
    void (*callback)(void *);
    void *id;
} PWM_Init_Config_s;
#endif

typedef enum
{
    Servo_None_Type = 0,
    Bus_Servo = 1,
    PWM_Servo = 2,
} ServoType_e;

typedef struct
{
    PWM_Init_Config_s pwm_init_config;
    ServoType_e servo_type;
    void *_handle;
    uint8_t servo_id;
} Servo_Init_Config_s;

typedef struct
{
    uint8_t servo_id;
    float angle;
    uint16_t recv_angle;
    PWMInstance *pwm_instance;
    USARTInstance *usart_instance;
    ServoType_e servo_type;
} ServoInstance;

ServoInstance *ServoInit(Servo_Init_Config_s *servo_init_config);
void ServoSetAngle(ServoInstance *servo, float angle);
uint8_t ServoControlInit(void);
void ServoControlTask(void);
void ServoControlSetTargetAngle(float angle);
float ServoControlGetTargetAngle(void);
float ServoControlGetCurrentAngle(void);
uint8_t ServoControlIsReady(void);

#endif /* SERVO_MOTOR_H */

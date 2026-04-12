#ifndef BSP_PWM_H_
#define BSP_PWM_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>

#include "tim.h"

#define PWM_DEVICE_CNT 16U

#ifndef RSCF_PWM_INSTANCE_FWD_DECLARED
#define RSCF_PWM_INSTANCE_FWD_DECLARED
typedef struct pwm_instance PWMInstance;
#endif

typedef void (*pwm_callback_t)(PWMInstance *instance);

struct pwm_instance
{
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    uint32_t tclk;
    float period;
    float dutyratio;
    pwm_callback_t callback;
    void *id;
    const struct device *dev;
    uint32_t channel_index;
    uint32_t period_cycles;
    uint8_t active;
    uint8_t zephyr_backend;
};

#ifndef RSCF_PWM_INIT_CONFIG_S_DEFINED
#define RSCF_PWM_INIT_CONFIG_S_DEFINED
typedef struct
{
    TIM_HandleTypeDef *htim;
    uint32_t channel;
    float period;
    float dutyratio;
    pwm_callback_t callback;
    void *id;
} PWM_Init_Config_s;
#endif

PWMInstance *PWMRegister(PWM_Init_Config_s *config);
void PWMStart(PWMInstance *pwm);
void PWMSetDutyRatio(PWMInstance *pwm, float dutyratio);
void PWMStop(PWMInstance *pwm);
void PWMSetPeriod(PWMInstance *pwm, float period);
void PWMStartDMA(PWMInstance *pwm, uint32_t *pData, uint32_t Size);

#endif /* BSP_PWM_H_ */

#include <stdlib.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/pwm.h>
#include <zephyr/logging/log.h>

#include "bsp_pwm.h"

LOG_MODULE_REGISTER(bsp_pwm, LOG_LEVEL_INF);

static PWMInstance *s_pwm_instances[PWM_DEVICE_CNT];
static uint8_t s_pwm_instance_count;

static const struct device *s_pwm4_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(pwm4));
static const struct device *s_pwm5_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(pwm5));
static const struct device *s_pwm8_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(pwm8));
static const struct device *s_pwm12_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(pwm12));

static void BSPPWMEnsureTimerDefaults(void)
{
    if (htim4.Instance == NULL)
    {
        memset(&htim4, 0, sizeof(htim4));
        htim4.Instance = TIM4;
        htim4.Init.Prescaler = 167U;
        htim4.Init.Period = 19999U;
    }

    if (htim5.Instance == NULL)
    {
        memset(&htim5, 0, sizeof(htim5));
        htim5.Instance = TIM5;
        htim5.Init.Prescaler = 167U;
        htim5.Init.Period = 19999U;
    }

    if (htim8.Instance == NULL)
    {
        memset(&htim8, 0, sizeof(htim8));
        htim8.Instance = TIM8;
        htim8.Init.Prescaler = 167U;
        htim8.Init.Period = 19999U;
    }

    if (htim12.Instance == NULL)
    {
        memset(&htim12, 0, sizeof(htim12));
        htim12.Instance = TIM12;
        htim12.Init.Prescaler = 83U;
        htim12.Init.Period = 369U;
    }
}

static const struct device *BSPPWMResolveDevice(TIM_HandleTypeDef *htim)
{
    if (htim == &htim4)
    {
        return s_pwm4_dev;
    }
    if (htim == &htim5)
    {
        return s_pwm5_dev;
    }
    if (htim == &htim8)
    {
        return s_pwm8_dev;
    }
    if (htim == &htim12)
    {
        return s_pwm12_dev;
    }

    return NULL;
}

static uint32_t BSPPWMResolveChannelIndex(uint32_t channel)
{
    switch (channel)
    {
    case TIM_CHANNEL_1:
        return 0U;
    case TIM_CHANNEL_2:
        return 1U;
    case TIM_CHANNEL_3:
        return 2U;
    case TIM_CHANNEL_4:
        return 3U;
    default:
        return UINT32_MAX;
    }
}

static void BSPPWMApply(PWMInstance *pwm)
{
    uint32_t pulse_cycles;

    if ((pwm == NULL) || (pwm->dev == NULL) || !device_is_ready(pwm->dev) || (pwm->period_cycles == 0U))
    {
        return;
    }

    pulse_cycles = (uint32_t)((float)pwm->period_cycles * pwm->dutyratio);
    (void)pwm_set_cycles(pwm->dev, pwm->channel_index, pwm->period_cycles, pulse_cycles, 0U);
}

PWMInstance *PWMRegister(PWM_Init_Config_s *config)
{
    PWMInstance *pwm;
    uint64_t cycles_per_sec = 0U;

    BSPPWMEnsureTimerDefaults();

    if ((config == NULL) || (config->htim == NULL) || (s_pwm_instance_count >= PWM_DEVICE_CNT))
    {
        return NULL;
    }

    pwm = (PWMInstance *)malloc(sizeof(PWMInstance));
    if (pwm == NULL)
    {
        return NULL;
    }

    memset(pwm, 0, sizeof(*pwm));
    pwm->htim = config->htim;
    pwm->channel = config->channel;
    pwm->period = config->period;
    pwm->dutyratio = config->dutyratio;
    pwm->callback = config->callback;
    pwm->id = config->id;
    pwm->channel_index = BSPPWMResolveChannelIndex(config->channel);
    pwm->dev = BSPPWMResolveDevice(config->htim);

    if ((pwm->dev == NULL) || !device_is_ready(pwm->dev) || (pwm->channel_index == UINT32_MAX))
    {
        LOG_WRN("pwm backend is not ready");
        free(pwm);
        return NULL;
    }

    if (pwm_get_cycles_per_sec(pwm->dev, pwm->channel_index, &cycles_per_sec) != 0)
    {
        free(pwm);
        return NULL;
    }

    pwm->tclk = (uint32_t)cycles_per_sec;
    pwm->zephyr_backend = 1U;
    pwm->active = 1U;
    s_pwm_instances[s_pwm_instance_count++] = pwm;

    PWMSetPeriod(pwm, pwm->period);
    PWMSetDutyRatio(pwm, pwm->dutyratio);
    return pwm;
}

void PWMStart(PWMInstance *pwm)
{
    if (pwm == NULL)
    {
        return;
    }

    pwm->active = 1U;
    BSPPWMApply(pwm);
}

void PWMStop(PWMInstance *pwm)
{
    if ((pwm == NULL) || (pwm->dev == NULL) || !device_is_ready(pwm->dev) || (pwm->period_cycles == 0U))
    {
        return;
    }

    pwm->active = 0U;
    (void)pwm_set_cycles(pwm->dev, pwm->channel_index, pwm->period_cycles, 0U, 0U);
}

void PWMSetPeriod(PWMInstance *pwm, float period)
{
    if ((pwm == NULL) || (pwm->tclk == 0U))
    {
        return;
    }

    pwm->period = period;
    pwm->period_cycles = (uint32_t)((double)pwm->tclk * (double)period);
    if (pwm->period_cycles == 0U)
    {
        pwm->period_cycles = 1U;
    }

    if (pwm->active != 0U)
    {
        BSPPWMApply(pwm);
    }
}

void PWMSetDutyRatio(PWMInstance *pwm, float dutyratio)
{
    if (pwm == NULL)
    {
        return;
    }

    if (dutyratio < 0.0f)
    {
        dutyratio = 0.0f;
    }
    else if (dutyratio > 1.0f)
    {
        dutyratio = 1.0f;
    }

    pwm->dutyratio = dutyratio;
    if (pwm->active != 0U)
    {
        BSPPWMApply(pwm);
    }
}

void PWMStartDMA(PWMInstance *pwm, uint32_t *pData, uint32_t Size)
{
    ARG_UNUSED(pData);
    ARG_UNUSED(Size);

    if ((pwm != NULL) && (pwm->callback != NULL))
    {
        pwm->callback(pwm);
    }
}

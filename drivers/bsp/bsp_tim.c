#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/logging/log.h>

#include <stm32_ll_bus.h>
#include <stm32_ll_gpio.h>
#include <stm32_ll_tim.h>

#include "bsp_tim.h"

LOG_MODULE_REGISTER(bsp_tim, LOG_LEVEL_INF);

TIM_HandleTypeDef htim2;
TIM_HandleTypeDef htim4;
TIM_HandleTypeDef htim5;
TIM_HandleTypeDef htim8;
TIM_HandleTypeDef htim12;

static TIMInstance *s_tim_instances[TIM_DEVICE_CNT];
static uint8_t s_tim_instance_count;
static bool s_tim_handles_ready;

static void BSPTIMEnsureHandles(void)
{
    if (s_tim_handles_ready)
    {
        return;
    }

    memset(&htim2, 0, sizeof(htim2));
    memset(&htim4, 0, sizeof(htim4));
    memset(&htim5, 0, sizeof(htim5));
    memset(&htim8, 0, sizeof(htim8));
    memset(&htim12, 0, sizeof(htim12));

    htim2.Instance = TIM2;
    htim2.Init.Prescaler = 0U;
    htim2.Init.Period = 0xFFFFFFFFU;

    htim4.Instance = TIM4;
    htim4.Init.Prescaler = 167U;
    htim4.Init.Period = 19999U;

    htim5.Instance = TIM5;
    htim5.Init.Prescaler = 167U;
    htim5.Init.Period = 19999U;

    htim8.Instance = TIM8;
    htim8.Init.Prescaler = 167U;
    htim8.Init.Period = 19999U;

    htim12.Instance = TIM12;
    htim12.Init.Prescaler = 83U;
    htim12.Init.Period = 369U;

    s_tim_handles_ready = true;
}

static void BSPTIMEnableClock(TIM_HandleTypeDef *htim)
{
    if (htim == NULL)
    {
        return;
    }

    if (htim->Instance == TIM2)
    {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM2);
    }
    else if (htim->Instance == TIM4)
    {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM4);
    }
    else if (htim->Instance == TIM5)
    {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM5);
    }
    else if (htim->Instance == TIM8)
    {
        LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_TIM8);
    }
    else if (htim->Instance == TIM12)
    {
        LL_APB1_GRP1_EnableClock(LL_APB1_GRP1_PERIPH_TIM12);
    }
}

static uint32_t BSPTIMResolveClockHz(TIM_HandleTypeDef *htim)
{
    if ((htim == &htim8))
    {
        return 168000000U;
    }

    return 84000000U;
}

static void BSPTIMConfigureTim2EncoderPins(void)
{
    LL_GPIO_InitTypeDef gpio_init = {0};

    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOA);

    gpio_init.Pin = LL_GPIO_PIN_0 | LL_GPIO_PIN_1;
    gpio_init.Mode = LL_GPIO_MODE_ALTERNATE;
    gpio_init.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpio_init.Pull = LL_GPIO_PULL_NO;
    gpio_init.Alternate = LL_GPIO_AF_1;
    LL_GPIO_Init(GPIOA, &gpio_init);
}

static uint32_t BSPTIMResolveLLChannel(uint32_t channel)
{
    switch (channel)
    {
    case TIM_CHANNEL_1:
        return LL_TIM_CHANNEL_CH1;
    case TIM_CHANNEL_2:
        return LL_TIM_CHANNEL_CH2;
    case TIM_CHANNEL_3:
        return LL_TIM_CHANNEL_CH3;
    case TIM_CHANNEL_4:
        return LL_TIM_CHANNEL_CH4;
    default:
        return 0U;
    }
}

TIMInstance *TIMRegister(TIM_Init_Config_s *config)
{
    TIMInstance *instance;

    BSPTIMEnsureHandles();

    if ((config == NULL) || (config->htim == NULL) || (s_tim_instance_count >= TIM_DEVICE_CNT))
    {
        return NULL;
    }

    if (config->htim->Instance == TIM_FORBIDDEN_INSTANCE)
    {
        return NULL;
    }

    instance = (TIMInstance *)malloc(sizeof(TIMInstance));
    if (instance == NULL)
    {
        return NULL;
    }

    memset(instance, 0, sizeof(*instance));
    instance->htim = config->htim;
    instance->mode = config->mode;
    instance->channels_mask = config->channels_mask;
    instance->period_callback = config->period_callback;
    instance->ic_callback = config->ic_callback;
    instance->oc_callback = config->oc_callback;
    instance->id = config->id;

    s_tim_instances[s_tim_instance_count++] = instance;
    return instance;
}

void TIMBaseStart_IT(TIMInstance *instance)
{
    if ((instance == NULL) || (instance->htim == NULL))
    {
        return;
    }

    BSPTIMEnableClock(instance->htim);
    LL_TIM_SetPrescaler(instance->htim->Instance, instance->htim->Init.Prescaler);
    LL_TIM_SetAutoReload(instance->htim->Instance, instance->htim->Init.Period);
    LL_TIM_EnableCounter(instance->htim->Instance);
}

void TIMBaseStop_IT(TIMInstance *instance)
{
    if ((instance == NULL) || (instance->htim == NULL))
    {
        return;
    }

    LL_TIM_DisableCounter(instance->htim->Instance);
}

void TIMICStart_IT(TIMInstance *instance, uint32_t channel)
{
    uint32_t ll_channel;

    if ((instance == NULL) || (instance->htim == NULL))
    {
        return;
    }

    ll_channel = BSPTIMResolveLLChannel(channel);
    if (ll_channel == 0U)
    {
        return;
    }

    BSPTIMEnableClock(instance->htim);
    LL_TIM_CC_EnableChannel(instance->htim->Instance, ll_channel);
    LL_TIM_EnableCounter(instance->htim->Instance);
}

void TIMICStop_IT(TIMInstance *instance, uint32_t channel)
{
    uint32_t ll_channel;

    if ((instance == NULL) || (instance->htim == NULL))
    {
        return;
    }

    ll_channel = BSPTIMResolveLLChannel(channel);
    if (ll_channel == 0U)
    {
        return;
    }

    LL_TIM_CC_DisableChannel(instance->htim->Instance, ll_channel);
}

void TIMOCStart_IT(TIMInstance *instance, uint32_t channel)
{
    TIMICStart_IT(instance, channel);
}

void TIMOCStop_IT(TIMInstance *instance, uint32_t channel)
{
    TIMICStop_IT(instance, channel);
}

void TIMSetPeriodUs(TIMInstance *instance, uint32_t period_us)
{
    uint32_t clk_hz;
    uint32_t psc;
    uint64_t arr;

    if ((instance == NULL) || (instance->htim == NULL))
    {
        return;
    }

    BSPTIMEnableClock(instance->htim);
    clk_hz = BSPTIMResolveClockHz(instance->htim);
    psc = instance->htim->Init.Prescaler + 1U;
    arr = ((uint64_t)period_us * (uint64_t)clk_hz) / ((uint64_t)psc * 1000000ULL);
    if (arr > 0U)
    {
        arr -= 1U;
    }

    instance->htim->Init.Period = (uint32_t)arr;
    LL_TIM_SetAutoReload(instance->htim->Instance, (uint32_t)arr);
    LL_TIM_SetCounter(instance->htim->Instance, 0U);
}

void TIMEncoderStart(TIM_HandleTypeDef *htim)
{
    BSPTIMEnsureHandles();

    if ((htim == NULL) || (htim != &htim2))
    {
        return;
    }

    BSPTIMEnableClock(htim);
    BSPTIMConfigureTim2EncoderPins();

    LL_TIM_DisableCounter(TIM2);
    LL_TIM_SetPrescaler(TIM2, 0U);
    LL_TIM_SetAutoReload(TIM2, 0xFFFFFFFFU);
    LL_TIM_SetEncoderMode(TIM2, LL_TIM_ENCODERMODE_X4_TI12);
    LL_TIM_IC_SetActiveInput(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_ACTIVEINPUT_DIRECTTI);
    LL_TIM_IC_SetActiveInput(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_ACTIVEINPUT_DIRECTTI);
    LL_TIM_IC_SetFilter(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_IC_FILTER_FDIV1);
    LL_TIM_IC_SetFilter(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_IC_FILTER_FDIV1);
    LL_TIM_IC_SetPrescaler(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_ICPSC_DIV1);
    LL_TIM_IC_SetPrescaler(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_ICPSC_DIV1);
    LL_TIM_IC_SetPolarity(TIM2, LL_TIM_CHANNEL_CH1, LL_TIM_IC_POLARITY_RISING);
    LL_TIM_IC_SetPolarity(TIM2, LL_TIM_CHANNEL_CH2, LL_TIM_IC_POLARITY_RISING);
    LL_TIM_CC_EnableChannel(TIM2, LL_TIM_CHANNEL_CH1 | LL_TIM_CHANNEL_CH2);
    LL_TIM_EnableCounter(TIM2);
}

void TIMEncoderStop(TIM_HandleTypeDef *htim)
{
    if ((htim == NULL) || (htim->Instance == NULL))
    {
        return;
    }

    LL_TIM_DisableCounter(htim->Instance);
}

uint32_t TIMGetCounter(TIMInstance *instance)
{
    if ((instance == NULL) || (instance->htim == NULL) || (instance->htim->Instance == NULL))
    {
        return 0U;
    }

    return LL_TIM_GetCounter(instance->htim->Instance);
}

void TIMSetCounter(TIMInstance *instance, uint32_t cnt)
{
    if ((instance == NULL) || (instance->htim == NULL) || (instance->htim->Instance == NULL))
    {
        return;
    }

    LL_TIM_SetCounter(instance->htim->Instance, cnt);
}

void BSP_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    for (uint8_t index = 0U; index < s_tim_instance_count; ++index)
    {
        if ((s_tim_instances[index] != NULL) &&
            (s_tim_instances[index]->htim == htim) &&
            (s_tim_instances[index]->period_callback != NULL))
        {
            s_tim_instances[index]->period_callback(s_tim_instances[index]);
            return;
        }
    }
}

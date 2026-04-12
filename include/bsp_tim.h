#ifndef BSP_TIM_H_
#define BSP_TIM_H_

#include <stdint.h>

#include "tim.h"

#define TIM_DEVICE_CNT 13U
#define TIM_FORBIDDEN_INSTANCE TIM14

typedef enum
{
    TIM_MODE_BASE = 0x01,
    TIM_MODE_INPUT_CAPTURE = 0x02,
    TIM_MODE_OUTPUT_COMPARE = 0x04,
} TIM_MODE_e;

struct tim_instance;

typedef void (*tim_period_callback_t)(struct tim_instance *instance);
typedef void (*tim_channel_callback_t)(struct tim_instance *instance, uint32_t channel);

typedef struct tim_instance
{
    TIM_HandleTypeDef *htim;
    TIM_MODE_e mode;
    uint32_t channels_mask;
    tim_period_callback_t period_callback;
    tim_channel_callback_t ic_callback;
    tim_channel_callback_t oc_callback;
    void *id;
} TIMInstance;

typedef struct
{
    TIM_HandleTypeDef *htim;
    TIM_MODE_e mode;
    uint32_t channels_mask;
    tim_period_callback_t period_callback;
    tim_channel_callback_t ic_callback;
    tim_channel_callback_t oc_callback;
    void *id;
} TIM_Init_Config_s;

TIMInstance *TIMRegister(TIM_Init_Config_s *config);
void TIMBaseStart_IT(TIMInstance *instance);
void TIMBaseStop_IT(TIMInstance *instance);
void TIMICStart_IT(TIMInstance *instance, uint32_t channel);
void TIMICStop_IT(TIMInstance *instance, uint32_t channel);
void TIMOCStart_IT(TIMInstance *instance, uint32_t channel);
void TIMOCStop_IT(TIMInstance *instance, uint32_t channel);
void TIMSetPeriodUs(TIMInstance *instance, uint32_t period_us);
void TIMEncoderStart(TIM_HandleTypeDef *htim);
void TIMEncoderStop(TIM_HandleTypeDef *htim);
uint32_t TIMGetCounter(TIMInstance *instance);
void TIMSetCounter(TIMInstance *instance, uint32_t cnt);
void BSP_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);

#endif /* BSP_TIM_H_ */

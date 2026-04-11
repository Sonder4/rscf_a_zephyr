#include <string.h>

#include <zephyr/kernel.h>

#include "mhall.h"

static MHallInstance s_instances[MHALL_DEVICE_CNT];
static uint8_t s_instance_count;

static int32_t MHallNormalizeDelta(int32_t raw_delta, uint32_t counter_period)
{
    int32_t counter_range;
    int32_t half_range;

    counter_range = (int32_t)(counter_period + 1U);
    half_range = counter_range / 2;
    if (raw_delta > half_range)
    {
        raw_delta -= counter_range;
    }
    else if (raw_delta < -half_range)
    {
        raw_delta += counter_range;
    }

    return raw_delta;
}

MHallInstance *MHallRegister(MHall_Init_Config_s *config)
{
    MHallInstance *instance;

    if ((config == NULL) || (config->counter_period == 0U) || (s_instance_count >= MHALL_DEVICE_CNT))
    {
        return NULL;
    }

    instance = &s_instances[s_instance_count++];
    memset(instance, 0, sizeof(*instance));
    instance->counter_period = config->counter_period;
    instance->reverse = config->reverse;
    instance->active = 1U;
    return instance;
}

void MHallFeedCount(MHallInstance *instance, int32_t current_counter)
{
    int32_t raw_delta;
    int32_t normalized_delta;

    if ((instance == NULL) || (instance->active == 0U))
    {
        return;
    }

    raw_delta = current_counter - instance->last_counter;
    normalized_delta = MHallNormalizeDelta(raw_delta, instance->counter_period);
    if (instance->reverse != 0U)
    {
        normalized_delta = -normalized_delta;
    }

    instance->data.delta_count = normalized_delta;
    instance->data.count += normalized_delta;
    instance->data.direction = (normalized_delta > 0) ? MHALL_DIRECTION_FORWARD :
                               (normalized_delta < 0) ? MHALL_DIRECTION_REVERSE :
                               MHALL_DIRECTION_STOP;
    instance->data.update_tick = k_uptime_get_32();
    instance->data.sample_seq++;
    instance->last_counter = current_counter;
}

void MHallReset(MHallInstance *instance)
{
    if (instance == NULL)
    {
        return;
    }

    memset(&instance->data, 0, sizeof(instance->data));
    instance->last_counter = 0;
}

void MHallUpdate(MHallInstance *instance)
{
    if (instance == NULL)
    {
        return;
    }

    instance->data.update_tick = k_uptime_get_32();
}

void MHallTask(void)
{
    for (uint8_t index = 0U; index < s_instance_count; ++index)
    {
        MHallUpdate(&s_instances[index]);
    }
}

int32_t MHallGetCount(MHallInstance *instance)
{
    return (instance != NULL) ? instance->data.count : 0;
}

MHallDirection_e MHallGetDirection(MHallInstance *instance)
{
    return (instance != NULL) ? (MHallDirection_e)instance->data.direction : MHALL_DIRECTION_STOP;
}

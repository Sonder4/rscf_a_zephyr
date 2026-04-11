#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>

#include "AD7190.h"

static AD7190_Frame_s s_ad7190_frame;
static const struct device *s_spi4_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(spi4));

int AD7190Init(void)
{
    memset(&s_ad7190_frame, 0, sizeof(s_ad7190_frame));
    s_ad7190_frame.ready = ((s_spi4_dev != NULL) && device_is_ready(s_spi4_dev)) ? 1U : 0U;
    s_ad7190_frame.update_tick = k_uptime_get_32();
    return 0;
}

void AD7190Task(void)
{
    s_ad7190_frame.update_tick = k_uptime_get_32();
}

uint8_t AD7190TryGetLatest(AD7190_Frame_s *out)
{
    if (out == NULL)
    {
        return 0U;
    }

    *out = s_ad7190_frame;
    return s_ad7190_frame.ready;
}

void AD7190SetMockVoltage(uint8_t channel, float voltage_mv)
{
    if (channel >= AD7190_CHANNEL_COUNT)
    {
        return;
    }

    s_ad7190_frame.voltage_mv[channel] = voltage_mv;
    s_ad7190_frame.sample_seq++;
    s_ad7190_frame.update_tick = k_uptime_get_32();
}

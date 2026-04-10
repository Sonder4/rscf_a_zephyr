#include "led.h"

#include "rscf_led_status.h"

void LED_CLOSE_ALL(void)
{
    RSCFLedStatusCloseAll();
}

void LedStatusInit(void)
{
    (void)RSCFLedStatusInit();
}

void LedStatusBeat(LedStatusChannel_e channel)
{
    RSCFLedStatusBeat((enum rscf_led_status_channel)channel);
}

void LedStatusSetHealthy(LedStatusChannel_e channel, uint8_t healthy)
{
    RSCFLedStatusSetHealthy((enum rscf_led_status_channel)channel, healthy != 0U);
}

void LedStatusMarkActivity(LedStatusChannel_e channel, uint16_t hold_ms)
{
    RSCFLedStatusMarkActivity((enum rscf_led_status_channel)channel, hold_ms);
}

void LedStatusRefresh10ms(void)
{
    RSCFLedStatusTick();
}

void LedStatusLatchFatal(void)
{
    RSCFLedStatusLatchFatal();
}

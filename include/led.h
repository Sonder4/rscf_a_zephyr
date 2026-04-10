#ifndef _LED_H
#define _LED_H

#include <stdint.h>

typedef enum
{
    LED_STATUS_BOOT = 0,
    LED_STATUS_MOTOR,
    LED_STATUS_DAEMON,
    LED_STATUS_ROBOT,
    LED_STATUS_CHASSIS,
    LED_STATUS_COMM,
    LED_STATUS_IMU,
    LED_STATUS_FAULT,
    LED_STATUS_CHANNEL_COUNT
} LedStatusChannel_e;

void LED_CLOSE_ALL(void);
void LedStatusInit(void);
void LedStatusBeat(LedStatusChannel_e channel);
void LedStatusSetHealthy(LedStatusChannel_e channel, uint8_t healthy);
void LedStatusMarkActivity(LedStatusChannel_e channel, uint16_t hold_ms);
void LedStatusRefresh10ms(void);
void LedStatusLatchFatal(void);

#endif

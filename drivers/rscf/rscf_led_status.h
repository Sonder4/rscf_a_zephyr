#ifndef RSCF_LED_STATUS_H_
#define RSCF_LED_STATUS_H_

#include <stdbool.h>
#include <stdint.h>

enum rscf_led_status_channel {
  RSCF_LED_STATUS_BOOT = 0,
  RSCF_LED_STATUS_MOTOR,
  RSCF_LED_STATUS_DAEMON,
  RSCF_LED_STATUS_ROBOT,
  RSCF_LED_STATUS_CHASSIS,
  RSCF_LED_STATUS_COMM,
  RSCF_LED_STATUS_IMU,
  RSCF_LED_STATUS_FAULT,
  RSCF_LED_STATUS_CHANNEL_COUNT,
};

int RSCFLedStatusInit(void);
void RSCFLedStatusBeat(enum rscf_led_status_channel channel);
void RSCFLedStatusSetHealthy(enum rscf_led_status_channel channel, bool healthy);
void RSCFLedStatusSetBlinkHalfPeriod(enum rscf_led_status_channel channel, uint16_t half_period_ms);
void RSCFLedStatusMarkActivity(enum rscf_led_status_channel channel, uint16_t hold_ms);
void RSCFLedStatusLatchFatal(void);
void RSCFLedStatusCloseAll(void);
void RSCFLedStatusTick(void);

#endif /* RSCF_LED_STATUS_H_ */

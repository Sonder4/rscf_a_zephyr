#include "rscf_led_status.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rscf_led_status, LOG_LEVEL_INF);

#define RSCF_LED_ACTIVITY_HALF_MS 250U
#define RSCF_LED_TASK_TIMEOUT_MS 200U
#define RSCF_LED_IMU_TIMEOUT_MS 200U

static const struct gpio_dt_spec s_status_leds[RSCF_LED_STATUS_CHANNEL_COUNT] = {
  [RSCF_LED_STATUS_BOOT] = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led1), gpios, {0}),
  [RSCF_LED_STATUS_MOTOR] = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led2), gpios, {0}),
  [RSCF_LED_STATUS_DAEMON] = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led3), gpios, {0}),
  [RSCF_LED_STATUS_ROBOT] = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led4), gpios, {0}),
  [RSCF_LED_STATUS_CHASSIS] = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led5), gpios, {0}),
  [RSCF_LED_STATUS_COMM] = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led6), gpios, {0}),
  [RSCF_LED_STATUS_IMU] = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led7), gpios, {0}),
  [RSCF_LED_STATUS_FAULT] = GPIO_DT_SPEC_GET_OR(DT_ALIAS(led8), gpios, {0}),
};
static bool s_ready[RSCF_LED_STATUS_CHANNEL_COUNT];
static bool s_health[RSCF_LED_STATUS_CHANNEL_COUNT];
static uint32_t s_last_beat_tick[RSCF_LED_STATUS_CHANNEL_COUNT];
static uint32_t s_activity_until_tick[RSCF_LED_STATUS_CHANNEL_COUNT];
static bool s_fatal_latched;
static const uint16_t s_beat_timeout_ms[RSCF_LED_STATUS_CHANNEL_COUNT] = {
  [RSCF_LED_STATUS_BOOT] = 0U,
  [RSCF_LED_STATUS_MOTOR] = RSCF_LED_TASK_TIMEOUT_MS,
  [RSCF_LED_STATUS_DAEMON] = RSCF_LED_TASK_TIMEOUT_MS,
  [RSCF_LED_STATUS_ROBOT] = RSCF_LED_TASK_TIMEOUT_MS,
  [RSCF_LED_STATUS_CHASSIS] = RSCF_LED_TASK_TIMEOUT_MS,
  [RSCF_LED_STATUS_COMM] = 0U,
  [RSCF_LED_STATUS_IMU] = RSCF_LED_IMU_TIMEOUT_MS,
  [RSCF_LED_STATUS_FAULT] = 0U,
};
static const bool s_activity_enabled[RSCF_LED_STATUS_CHANNEL_COUNT] = {
  [RSCF_LED_STATUS_BOOT] = false,
  [RSCF_LED_STATUS_MOTOR] = false,
  [RSCF_LED_STATUS_DAEMON] = false,
  [RSCF_LED_STATUS_ROBOT] = false,
  [RSCF_LED_STATUS_CHASSIS] = false,
  [RSCF_LED_STATUS_COMM] = true,
  [RSCF_LED_STATUS_IMU] = true,
  [RSCF_LED_STATUS_FAULT] = false,
};

static bool RSCFLedStatusActivityActive(uint32_t now_ms, uint32_t until_ms)
{
  return ((int32_t)(until_ms - now_ms) > 0) ? true : false;
}

static bool RSCFLedStatusChannelValid(enum rscf_led_status_channel channel)
{
  return (channel >= RSCF_LED_STATUS_BOOT) && (channel < RSCF_LED_STATUS_CHANNEL_COUNT);
}

static void RSCFLedStatusWrite(enum rscf_led_status_channel channel, bool on)
{
  if (!RSCFLedStatusChannelValid(channel) || !s_ready[channel]) {
    return;
  }

  gpio_pin_set_dt(&s_status_leds[channel], on ? 1 : 0);
}

int RSCFLedStatusInit(void)
{
  for (size_t i = 0; i < ARRAY_SIZE(s_health); ++i) {
    s_health[i] = false;
    s_last_beat_tick[i] = 0U;
    s_activity_until_tick[i] = 0U;
  }

  for (size_t i = 0; i < ARRAY_SIZE(s_status_leds); ++i) {
    if (!device_is_ready(s_status_leds[i].port)) {
      LOG_WRN("status led %u is not ready", (unsigned int)i);
      s_ready[i] = false;
      continue;
    }

    (void)gpio_pin_configure_dt(&s_status_leds[i], GPIO_OUTPUT_INACTIVE);
    s_ready[i] = true;
  }

  s_health[RSCF_LED_STATUS_BOOT] = true;
  s_health[RSCF_LED_STATUS_FAULT] = true;
  s_last_beat_tick[RSCF_LED_STATUS_BOOT] = k_uptime_get_32();
  s_last_beat_tick[RSCF_LED_STATUS_FAULT] = s_last_beat_tick[RSCF_LED_STATUS_BOOT];
  s_fatal_latched = false;
  LOG_INF("led status initialized");
  return 0;
}

void RSCFLedStatusBeat(enum rscf_led_status_channel channel)
{
  if (!RSCFLedStatusChannelValid(channel)) {
    return;
  }

  s_health[channel] = true;
  s_last_beat_tick[channel] = k_uptime_get_32();
}

void RSCFLedStatusSetHealthy(enum rscf_led_status_channel channel, bool healthy)
{
  if (!RSCFLedStatusChannelValid(channel)) {
    return;
  }

  s_health[channel] = healthy;
  if (healthy) {
    s_last_beat_tick[channel] = k_uptime_get_32();
  }
}

void RSCFLedStatusMarkActivity(enum rscf_led_status_channel channel, uint16_t hold_ms)
{
  if (!RSCFLedStatusChannelValid(channel) || (hold_ms == 0U)) {
    return;
  }

  s_activity_until_tick[channel] = k_uptime_get_32() + (uint32_t)hold_ms;
}

void RSCFLedStatusLatchFatal(void)
{
  s_fatal_latched = true;
  s_health[RSCF_LED_STATUS_FAULT] = false;
  RSCFLedStatusWrite(RSCF_LED_STATUS_FAULT, false);
}

void RSCFLedStatusCloseAll(void)
{
  for (size_t i = 0; i < ARRAY_SIZE(s_health); ++i) {
    s_health[i] = false;
    s_last_beat_tick[i] = 0U;
    s_activity_until_tick[i] = 0U;
    RSCFLedStatusWrite((enum rscf_led_status_channel)i, false);
  }
}

void RSCFLedStatusTick(void)
{
  uint32_t now_ms;
  bool blink_on;

  now_ms = k_uptime_get_32();
  blink_on = (((now_ms / RSCF_LED_ACTIVITY_HALF_MS) & 0x01U) == 0U);

  for (size_t i = 0; i < ARRAY_SIZE(s_health); ++i) {
    bool led_on;

    if ((s_beat_timeout_ms[i] > 0U) && s_health[i]) {
      if ((uint32_t)(now_ms - s_last_beat_tick[i]) > s_beat_timeout_ms[i]) {
        s_health[i] = false;
      }
    }

    led_on = s_health[i];
    if (led_on && s_activity_enabled[i] &&
        RSCFLedStatusActivityActive(now_ms, s_activity_until_tick[i])) {
      led_on = blink_on;
    }

    if ((i == RSCF_LED_STATUS_FAULT) && s_fatal_latched) {
      led_on = false;
    }

    RSCFLedStatusWrite((enum rscf_led_status_channel)i, led_on);
  }
}

#include "rscf_led_status.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rscf_led_status, LOG_LEVEL_INF);

#define RSCF_STATUS_LED_NODE DT_ALIAS(led0)
#define RSCF_LED_ACTIVITY_HALF_MS 250U
#define RSCF_LED_TASK_TIMEOUT_MS 200U
#define RSCF_LED_IMU_TIMEOUT_MS 200U

static struct gpio_dt_spec s_status_led = GPIO_DT_SPEC_GET_OR(
  RSCF_STATUS_LED_NODE,
  gpios,
  {0}
);
static bool s_ready;
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

int RSCFLedStatusInit(void)
{
  for (size_t i = 0; i < ARRAY_SIZE(s_health); ++i) {
    s_health[i] = false;
    s_last_beat_tick[i] = 0U;
    s_activity_until_tick[i] = 0U;
  }

  if (!device_is_ready(s_status_led.port)) {
    LOG_WRN("status led is not ready");
    s_ready = false;
    return 0;
  }

  gpio_pin_configure_dt(&s_status_led, GPIO_OUTPUT_INACTIVE);
  s_ready = true;
  s_health[RSCF_LED_STATUS_BOOT] = true;
  s_last_beat_tick[RSCF_LED_STATUS_BOOT] = k_uptime_get_32();
  s_fatal_latched = false;
  LOG_INF("led status initialized");
  return 0;
}

void RSCFLedStatusBeat(enum rscf_led_status_channel channel)
{
  if ((channel < 0) || (channel >= RSCF_LED_STATUS_CHANNEL_COUNT)) {
    return;
  }

  s_health[channel] = true;
  s_last_beat_tick[channel] = k_uptime_get_32();
}

void RSCFLedStatusSetHealthy(enum rscf_led_status_channel channel, bool healthy)
{
  if ((channel < 0) || (channel >= RSCF_LED_STATUS_CHANNEL_COUNT)) {
    return;
  }

  s_health[channel] = healthy;
  if (healthy) {
    s_last_beat_tick[channel] = k_uptime_get_32();
  }
}

void RSCFLedStatusMarkActivity(enum rscf_led_status_channel channel, uint16_t hold_ms)
{
  if ((channel < 0) || (channel >= RSCF_LED_STATUS_CHANNEL_COUNT) || (hold_ms == 0U)) {
    return;
  }

  s_activity_until_tick[channel] = k_uptime_get_32() + (uint32_t)hold_ms;
}

void RSCFLedStatusLatchFatal(void)
{
  s_fatal_latched = true;
  s_health[RSCF_LED_STATUS_FAULT] = false;

  if (s_ready) {
    gpio_pin_set_dt(&s_status_led, 1);
  }
}

void RSCFLedStatusCloseAll(void)
{
  for (size_t i = 0; i < ARRAY_SIZE(s_health); ++i) {
    s_health[i] = false;
    s_last_beat_tick[i] = 0U;
    s_activity_until_tick[i] = 0U;
  }

  if (s_ready) {
    gpio_pin_set_dt(&s_status_led, 1);
  }
}

void RSCFLedStatusTick(void)
{
  bool any_healthy = false;
  bool blink_enabled = false;
  bool blink_on = false;
  uint32_t now_ms;

  if (!s_ready) {
    return;
  }

  now_ms = k_uptime_get_32();
  blink_on = (((now_ms / RSCF_LED_ACTIVITY_HALF_MS) & 0x01U) == 0U);

  for (size_t i = 0; i < ARRAY_SIZE(s_health); ++i) {
    if ((s_beat_timeout_ms[i] > 0U) && s_health[i]) {
      if ((uint32_t)(now_ms - s_last_beat_tick[i]) > s_beat_timeout_ms[i]) {
        s_health[i] = false;
      }
    }

    if (s_health[i]) {
      any_healthy = true;
      if (s_activity_enabled[i] &&
          RSCFLedStatusActivityActive(now_ms, s_activity_until_tick[i])) {
        blink_enabled = true;
      }
    }
  }

  if (s_fatal_latched) {
    gpio_pin_set_dt(&s_status_led, 1);
    return;
  }

  if (!any_healthy) {
    gpio_pin_set_dt(&s_status_led, 1);
    return;
  }

  gpio_pin_set_dt(&s_status_led, (blink_enabled && !blink_on) ? 1 : 0);
}

#include "rscf_led_status.h"

#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rscf_led_status, LOG_LEVEL_INF);

#define RSCF_STATUS_LED_NODE DT_ALIAS(led0)

static struct gpio_dt_spec s_status_led = GPIO_DT_SPEC_GET_OR(
  RSCF_STATUS_LED_NODE,
  gpios,
  {0}
);
static bool s_ready;
static uint32_t s_beat_count[RSCF_LED_STATUS_CHANNEL_COUNT];
static bool s_health[RSCF_LED_STATUS_CHANNEL_COUNT];

int RSCFLedStatusInit(void)
{
  for (size_t i = 0; i < ARRAY_SIZE(s_beat_count); ++i) {
    s_beat_count[i] = 0U;
    s_health[i] = false;
  }

  if (!device_is_ready(s_status_led.port)) {
    LOG_WRN("status led is not ready");
    s_ready = false;
    return 0;
  }

  gpio_pin_configure_dt(&s_status_led, GPIO_OUTPUT_INACTIVE);
  s_ready = true;
  s_health[RSCF_LED_STATUS_BOOT] = true;
  LOG_INF("led status initialized");
  return 0;
}

void RSCFLedStatusBeat(enum rscf_led_status_channel channel)
{
  if ((channel < 0) || (channel >= RSCF_LED_STATUS_CHANNEL_COUNT)) {
    return;
  }

  s_beat_count[channel]++;
  s_health[channel] = true;
}

void RSCFLedStatusSetHealthy(enum rscf_led_status_channel channel, bool healthy)
{
  if ((channel < 0) || (channel >= RSCF_LED_STATUS_CHANNEL_COUNT)) {
    return;
  }

  s_health[channel] = healthy;
}

void RSCFLedStatusTick(void)
{
  bool any_healthy = false;

  if (!s_ready) {
    return;
  }

  for (size_t i = 0; i < ARRAY_SIZE(s_health); ++i) {
    if (s_health[i]) {
      any_healthy = true;
      break;
    }
  }

  gpio_pin_set_dt(&s_status_led, any_healthy ? 0 : 1);
}

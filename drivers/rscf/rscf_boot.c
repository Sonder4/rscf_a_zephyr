#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "bsp_dwt.h"
#include "bsp_log.h"
#include <rscf/rscf_boot.h>

LOG_MODULE_REGISTER(rscf_boot, LOG_LEVEL_INF);

#define RSCF_STATUS_LED_NODE DT_ALIAS(led0)

static bool s_bsp_ready;

int RSCFBootRun(void)
{
#if DT_NODE_HAS_STATUS(RSCF_STATUS_LED_NODE, okay)
  static const struct gpio_dt_spec status_led =
    GPIO_DT_SPEC_GET(RSCF_STATUS_LED_NODE, gpios);

  if (!gpio_is_ready_dt(&status_led)) {
    LOG_WRN("status led device not ready");
    return -ENODEV;
  }

  gpio_pin_configure_dt(&status_led, GPIO_OUTPUT_INACTIVE);
  gpio_pin_set_dt(&status_led, 1);
  k_sleep(K_MSEC(50));
  gpio_pin_set_dt(&status_led, 0);
#else
  LOG_WRN("status led alias is not defined");
#endif

  BSPLogInit();
  DWT_Init(0U);
  s_bsp_ready = true;

  LOG_INF("board bring-up baseline completed");
  return 0;
}

bool RSCFBootBspReady(void)
{
  return s_bsp_ready;
}

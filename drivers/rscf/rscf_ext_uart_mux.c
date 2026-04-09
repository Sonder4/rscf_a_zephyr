#include "rscf_ext_uart_mux.h"

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rscf_ext_uart_mux, LOG_LEVEL_INF);

#define RSCF_EXT_UART_MUX_NODE DT_NODELABEL(rscf_ext_uart_mux)

#if DT_NODE_HAS_STATUS(RSCF_EXT_UART_MUX_NODE, okay)
static struct gpio_dt_spec s_mux_a = GPIO_DT_SPEC_GET(RSCF_EXT_UART_MUX_NODE, a_gpios);
static struct gpio_dt_spec s_mux_b = GPIO_DT_SPEC_GET(RSCF_EXT_UART_MUX_NODE, b_gpios);
static const struct device *s_mux_uart = DEVICE_DT_GET(DT_PHANDLE(RSCF_EXT_UART_MUX_NODE, uart));
static const uint32_t s_switch_settle_ms = DT_PROP(RSCF_EXT_UART_MUX_NODE, switch_settle_ms);
static uint8_t s_current_channel = DT_PROP(RSCF_EXT_UART_MUX_NODE, default_channel);
static bool s_ready;
#endif

static int rscf_ext_uart_mux_apply(uint8_t channel)
{
#if DT_NODE_HAS_STATUS(RSCF_EXT_UART_MUX_NODE, okay)
  if (!s_ready) {
    return -ENODEV;
  }

  if (channel > 3U) {
    return -EINVAL;
  }

  gpio_pin_set_dt(&s_mux_a, (channel & 0x1U) != 0U);
  gpio_pin_set_dt(&s_mux_b, (channel & 0x2U) != 0U);
  s_current_channel = channel;
  return 0;
#else
  ARG_UNUSED(channel);
  return 0;
#endif
}

int RSCFExtUartMuxInit(void)
{
#if DT_NODE_HAS_STATUS(RSCF_EXT_UART_MUX_NODE, okay)
  if (!device_is_ready(s_mux_uart) ||
      !gpio_is_ready_dt(&s_mux_a) ||
      !gpio_is_ready_dt(&s_mux_b)) {
    LOG_WRN("ext uart mux backing devices are not ready");
    s_ready = false;
    return 0;
  }

  gpio_pin_configure_dt(&s_mux_a, GPIO_OUTPUT_INACTIVE);
  gpio_pin_configure_dt(&s_mux_b, GPIO_OUTPUT_INACTIVE);
  s_ready = true;
  rscf_ext_uart_mux_apply(s_current_channel);
  LOG_INF("ext uart mux initialized on channel %u", s_current_channel);
  return 0;
#else
  LOG_INF("ext uart mux node disabled");
  return 0;
#endif
}

int RSCFExtUartMuxSelect(uint8_t channel)
{
  int ret = rscf_ext_uart_mux_apply(channel);

#if DT_NODE_HAS_STATUS(RSCF_EXT_UART_MUX_NODE, okay)
  if ((ret == 0) && (s_switch_settle_ms > 0U)) {
    k_msleep(s_switch_settle_ms);
  }
#endif

  return ret;
}

uint8_t RSCFExtUartMuxCurrentChannel(void)
{
#if DT_NODE_HAS_STATUS(RSCF_EXT_UART_MUX_NODE, okay)
  return s_current_channel;
#else
  return 0U;
#endif
}

const struct device *RSCFExtUartMuxUart(void)
{
#if DT_NODE_HAS_STATUS(RSCF_EXT_UART_MUX_NODE, okay)
  return s_mux_uart;
#else
  return NULL;
#endif
}

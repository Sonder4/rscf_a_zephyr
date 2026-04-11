#include "rscf_comm_service.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "rscf_led_status.h"
#include "robot_interface.h"
#include "transport_uart.h"
#include "transport_usb.h"

LOG_MODULE_REGISTER(rscf_comm_service, LOG_LEVEL_INF);

#define RSCF_BOARD_NODE DT_NODELABEL(rscf_board)
#define RSCF_COMM_LED_FAST_HALF_MS 100U
#define RSCF_COMM_LED_SLOW_HALF_MS 500U
#define RSCF_COMM_LED_FAST_HOLD_MS 300U

static bool s_comm_ready;
static Transport_interface_t *s_selected_transport;
static uint32_t s_last_valid_rx_tick_ms;

static void RSCFCommServiceUpdateLed(uint32_t now_ms)
{
#if defined(CONFIG_RSCF_LED_STATUS)
  uint16_t blink_half_ms = RSCF_COMM_LED_SLOW_HALF_MS;

  if ((s_last_valid_rx_tick_ms != 0U) &&
      ((uint32_t)(now_ms - s_last_valid_rx_tick_ms) <= RSCF_COMM_LED_FAST_HOLD_MS)) {
    blink_half_ms = RSCF_COMM_LED_FAST_HALF_MS;
  }

  RSCFLedStatusSetHealthy(RSCF_LED_STATUS_COMM, true);
  RSCFLedStatusSetBlinkHalfPeriod(RSCF_LED_STATUS_COMM, blink_half_ms);
#else
  ARG_UNUSED(now_ms);
#endif
}

void Comm_OnValidRxFrame(uint8_t pid)
{
  ARG_UNUSED(pid);
  s_last_valid_rx_tick_ms = k_uptime_get_32();
}

int RSCFCommServiceInit(void)
{
  const struct device *can_primary = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, can_primary));
  const struct device *can_secondary = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, can_secondary));
  bool transport_ready = false;
  int ret;

  s_selected_transport = &g_uart_transport;
#if (MCU_COMM_DEFAULT_TRANSPORT == MCU_COMM_TRANSPORT_USB) && \
    defined(CONFIG_USB_DEVICE_STACK) && DT_HAS_COMPAT_STATUS_OKAY(zephyr_cdc_acm_uart)
  {
    const struct device *usb_uart = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
    transport_ready = device_is_ready(usb_uart);
    s_selected_transport = &g_usb_transport;
  }
#else
  {
    const struct device *comm_uart = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, comm_uart));
    transport_ready = device_is_ready(comm_uart);
  }
#endif

  s_comm_ready = transport_ready &&
                 device_is_ready(can_primary) &&
                 device_is_ready(can_secondary);
  s_last_valid_rx_tick_ms = 0U;
  RSCFCommServiceUpdateLed(k_uptime_get_32());
  if (!s_comm_ready) {
    LOG_WRN("comm devices are not ready");
    return 0;
  }

  RobotInterfaceSetTransport(s_selected_transport);
  ret = RobotInterfaceInit() ? 0 : -ENODEV;
  if (ret != 0) {
    s_comm_ready = false;
    return ret;
  }

  LOG_INF("comm service ready=%d transport=%s",
          s_comm_ready ? 1 : 0,
          (s_selected_transport == &g_usb_transport) ? "usb" : "uart");
  return 0;
}

void RSCFCommServiceProcess(void)
{
  if (!s_comm_ready) {
    RSCFCommServiceUpdateLed(k_uptime_get_32());
    return;
  }

  RobotInterfaceProcess();
  RSCFCommServiceUpdateLed(k_uptime_get_32());
}

bool RSCFCommServiceReady(void)
{
  return s_comm_ready;
}

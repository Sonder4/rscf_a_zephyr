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

extern Transport_interface_t *RSCFLinkUsbAdapterGetTransport(void);
extern bool RSCFLinkUsbAdapterIsReady(void);
extern Transport_interface_t *RSCFLinkUartAdapterGetTransport(void);
extern bool RSCFLinkUartAdapterIsReady(void);
extern Transport_interface_t *RSCFLinkCanAdapterGetTransport(void);
extern bool RSCFLinkCanAdapterIsReady(void);
extern Transport_interface_t *RSCFLinkSpiAdapterGetTransport(void);
extern bool RSCFLinkSpiAdapterIsReady(void);

static bool s_comm_ready;
static Transport_interface_t *s_selected_transport;
static uint32_t s_last_valid_rx_tick_ms;

static const char *RSCFCommServiceSelectAdapterRole(Transport_interface_t *transport)
{
  if (transport == RSCFLinkUsbAdapterGetTransport()) {
    return "host_usb";
  }

  if (transport == RSCFLinkUartAdapterGetTransport()) {
    return "peer_uart";
  }

  if (transport == RSCFLinkCanAdapterGetTransport()) {
    return "peer_can";
  }

  if (transport == RSCFLinkSpiAdapterGetTransport()) {
    return "peer_spi";
  }

  return "none";
}

static Transport_interface_t *RSCFCommServiceSelectTransport(bool *transport_ready,
                                                             const char **adapter_role)
{
  Transport_interface_t *transport = NULL;
  bool ready = false;

#if MCU_COMM_DEFAULT_TRANSPORT == MCU_COMM_TRANSPORT_CAN
  transport = RSCFLinkCanAdapterGetTransport();
  ready = RSCFLinkCanAdapterIsReady();
#elif MCU_COMM_DEFAULT_TRANSPORT == MCU_COMM_TRANSPORT_UART
  transport = RSCFLinkUartAdapterGetTransport();
  ready = RSCFLinkUartAdapterIsReady();
#else
  transport = RSCFLinkUsbAdapterGetTransport();
  ready = RSCFLinkUsbAdapterIsReady();
#endif

  if (!ready) {
    transport = RSCFLinkUartAdapterGetTransport();
    ready = RSCFLinkUartAdapterIsReady();
  }

  if (!ready) {
    transport = RSCFLinkCanAdapterGetTransport();
    ready = RSCFLinkCanAdapterIsReady();
  }

  if (!ready) {
    transport = RSCFLinkSpiAdapterGetTransport();
    ready = (transport != NULL) && RSCFLinkSpiAdapterIsReady();
  }

  if (transport_ready != NULL) {
    *transport_ready = ready;
  }

  if (adapter_role != NULL) {
    *adapter_role = RSCFCommServiceSelectAdapterRole(transport);
  }

  return transport;
}

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
  bool transport_ready = false;
  const char *adapter_role = "none";
  int ret;

  /* 这里只做 host/peer 传输骨架选择，不引入 Task 5 的桥接逻辑。 */
  s_selected_transport = RSCFCommServiceSelectTransport(&transport_ready, &adapter_role);
  s_comm_ready = transport_ready && (s_selected_transport != NULL);
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

  LOG_INF("comm service ready=%d transport=%s role=%s",
          s_comm_ready ? 1 : 0,
          (s_selected_transport == &g_usb_transport) ? "usb" :
          (s_selected_transport == &g_can_transport) ? "can" :
          (s_selected_transport == &g_uart_transport) ? "uart" : "none",
          adapter_role);
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

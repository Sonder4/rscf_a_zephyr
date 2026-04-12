#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include "transport_uart.h"

Transport_interface_t *RSCFLinkUartAdapterGetTransport(void)
{
  return &g_uart_transport;
}

bool RSCFLinkUartAdapterIsReady(void)
{
#if DT_NODE_EXISTS(DT_CHOSEN(rscf_peer_link)) && DT_NODE_HAS_STATUS(DT_CHOSEN(rscf_peer_link), okay)
  const struct device *peer_uart = DEVICE_DT_GET(DT_CHOSEN(rscf_peer_link));

  return device_is_ready(peer_uart);
#else
  return false;
#endif
}

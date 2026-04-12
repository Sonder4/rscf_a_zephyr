#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include "rscf_link_adapter.h"
#include "transport_uart.h"

static bool rscf_link_uart_is_ready(void)
{
#if DT_NODE_EXISTS(DT_CHOSEN(rscf_peer_link)) && DT_NODE_HAS_STATUS(DT_CHOSEN(rscf_peer_link), okay)
  const struct device *peer_uart = DEVICE_DT_GET(DT_CHOSEN(rscf_peer_link));

  return device_is_ready(peer_uart);
#else
  return false;
#endif
}

static const struct rscf_link_adapter s_uart_adapter = {
  .name = "uart",
  .role = "peer",
  .transport = &g_uart_transport,
  .is_ready = rscf_link_uart_is_ready,
};

const struct rscf_link_adapter *RSCFLinkUartAdapterGet(void)
{
  return &s_uart_adapter;
}

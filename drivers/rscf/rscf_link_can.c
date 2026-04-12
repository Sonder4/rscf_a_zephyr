#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include "transport_can.h"

#define RSCF_BOARD_NODE DT_NODELABEL(rscf_board)

Transport_interface_t *RSCFLinkCanAdapterGetTransport(void)
{
  return &g_can_transport;
}

bool RSCFLinkCanAdapterIsReady(void)
{
#if DT_NODE_EXISTS(RSCF_BOARD_NODE)
  const struct device *can_primary = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, can_primary));

  return device_is_ready(can_primary);
#else
  return false;
#endif
}

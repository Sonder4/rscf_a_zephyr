#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include "rscf_link_adapter.h"
#include "transport_can.h"

#define RSCF_BOARD_NODE DT_NODELABEL(rscf_board)

static bool rscf_link_can_is_ready(void)
{
#if DT_NODE_EXISTS(RSCF_BOARD_NODE)
  const struct device *can_primary = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, can_primary));

  return device_is_ready(can_primary);
#else
  return false;
#endif
}

static const struct rscf_link_adapter s_can_adapter = {
  .name = "can",
  .role = "peer",
  .transport = &g_can_transport,
  .is_ready = rscf_link_can_is_ready,
};

const struct rscf_link_adapter *RSCFLinkCanAdapterGet(void)
{
  return &s_can_adapter;
}

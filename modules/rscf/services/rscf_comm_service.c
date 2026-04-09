#include "rscf_comm_service.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rscf_comm_service, LOG_LEVEL_INF);

#define RSCF_BOARD_NODE DT_NODELABEL(rscf_board)

static bool s_comm_ready;

int RSCFCommServiceInit(void)
{
  const struct device *console_uart = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, console_uart));
  const struct device *can_primary = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, can_primary));
  const struct device *can_secondary = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, can_secondary));

  s_comm_ready = device_is_ready(console_uart) &&
                 device_is_ready(can_primary) &&
                 device_is_ready(can_secondary);

  LOG_INF("comm service ready=%d", s_comm_ready ? 1 : 0);
  return 0;
}

bool RSCFCommServiceReady(void)
{
  return s_comm_ready;
}

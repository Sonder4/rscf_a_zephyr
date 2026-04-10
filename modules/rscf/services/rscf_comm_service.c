#include "rscf_comm_service.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include "robot_interface.h"

LOG_MODULE_REGISTER(rscf_comm_service, LOG_LEVEL_INF);

#define RSCF_BOARD_NODE DT_NODELABEL(rscf_board)

static bool s_comm_ready;

int RSCFCommServiceInit(void)
{
  const struct device *comm_uart = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, comm_uart));
  const struct device *can_primary = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, can_primary));
  const struct device *can_secondary = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, can_secondary));
  int ret;

  s_comm_ready = device_is_ready(comm_uart) &&
                 device_is_ready(can_primary) &&
                 device_is_ready(can_secondary);
  if (!s_comm_ready) {
    LOG_WRN("comm devices are not ready");
    return 0;
  }

  ret = RobotInterfaceInit() ? 0 : -ENODEV;
  if (ret != 0) {
    s_comm_ready = false;
    return ret;
  }

  LOG_INF("comm service ready=%d", s_comm_ready ? 1 : 0);
  return 0;
}

void RSCFCommServiceProcess(void)
{
  if (!s_comm_ready) {
    return;
  }

  RobotInterfaceProcess();
}

bool RSCFCommServiceReady(void)
{
  return s_comm_ready;
}

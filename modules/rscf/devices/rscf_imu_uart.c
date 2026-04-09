#include "rscf_imu_uart.h"

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rscf_imu_uart, LOG_LEVEL_INF);

#define RSCF_BOARD_NODE DT_NODELABEL(rscf_board)

static bool s_imu_ready;

int RSCFImuUartInit(void)
{
  const struct device *imu_uart = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, imu_uart));

  s_imu_ready = device_is_ready(imu_uart);
  LOG_INF("imu uart ready=%d", s_imu_ready ? 1 : 0);
  return 0;
}

bool RSCFImuUartReady(void)
{
  return s_imu_ready;
}

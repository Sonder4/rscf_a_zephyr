#include "rscf_imu_uart.h"

#include <errno.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

#include <rscf/rscf_daemon_service.h>
#include <rscf/rscf_event_bus.h>

#include "rscf_led_status.h"

LOG_MODULE_REGISTER(rscf_imu_uart, LOG_LEVEL_INF);

#define RSCF_BOARD_NODE DT_NODELABEL(rscf_board)
#define RSCF_SABER_FRAME_MIN_SIZE 8U

enum rscf_saber_pid {
  RSCF_SABER_PID_KALMAN_ACC = 0x8801,
  RSCF_SABER_PID_KALMAN_GYRO = 0x8C01,
  RSCF_SABER_PID_EULER = 0xB001,
};

static bool s_imu_ready;
static bool s_imu_has_frame;
static uint32_t s_imu_sample_seq;
static struct rscf_saber_topic_data s_imu_latest;
static struct rscf_event_bus_publisher s_imu_publisher;
static struct rscf_daemon_handle *s_imu_daemon;

static void rscf_imu_uart_daemon_callback(void *owner, enum rscf_daemon_event event)
{
  ARG_UNUSED(owner);

  if (event == RSCF_DAEMON_EVENT_OFFLINE_ENTER) {
    RSCFLedStatusSetHealthy(RSCF_LED_STATUS_IMU, false);
    LOG_WRN("imu stream offline");
  } else if (event == RSCF_DAEMON_EVENT_ONLINE_RECOVER) {
    RSCFLedStatusSetHealthy(RSCF_LED_STATUS_IMU, true);
    LOG_INF("imu stream recovered");
  }
}

static bool rscf_imu_uart_decode(const uint8_t *data, size_t len)
{
  size_t index = 6U;
  bool updated = false;

  while ((len > (index + 6U)) && ((index + 16U) < len)) {
    uint16_t pid = (uint16_t)data[index] | ((uint16_t)data[index + 1U] << 8);

    index++;
    switch (pid) {
      case RSCF_SABER_PID_KALMAN_ACC:
        index += 2U;
        memcpy(&s_imu_latest.kalman_acc.x, &data[index], sizeof(s_imu_latest.kalman_acc));
        updated = true;
        break;

      case RSCF_SABER_PID_KALMAN_GYRO:
        index += 2U;
        memcpy(&s_imu_latest.kalman_gyro.x, &data[index], sizeof(s_imu_latest.kalman_gyro));
        updated = true;
        break;

      case RSCF_SABER_PID_EULER:
        index += 2U;
        memcpy(&s_imu_latest.euler.roll, &data[index], sizeof(s_imu_latest.euler));
        updated = true;
        break;

      default:
        break;
    }

    index += 12U;
  }

  return updated;
}

int RSCFImuUartInit(void)
{
  const struct device *imu_uart = DEVICE_DT_GET(DT_PHANDLE(RSCF_BOARD_NODE, imu_uart));
  int ret;

  s_imu_ready = device_is_ready(imu_uart);
  s_imu_has_frame = false;
  s_imu_sample_seq = 0U;
  memset(&s_imu_latest, 0, sizeof(s_imu_latest));
  memset(&s_imu_publisher, 0, sizeof(s_imu_publisher));
  s_imu_daemon = NULL;

  if (!s_imu_ready) {
    LOG_WRN("imu uart backend is not ready");
    return 0;
  }

  ret = RSCFEventBusPublisherRegister(
    RSCF_SABER_IMU_TOPIC_NAME,
    sizeof(s_imu_latest),
    &s_imu_publisher
  );
  if (ret != 0) {
    return ret;
  }

  ret = RSCFDaemonRegister(
    &(const struct rscf_daemon_config){
      .reload_ticks = 100U,
      .init_ticks = 100U,
      .remind_ticks = 1500U,
      .callback = rscf_imu_uart_daemon_callback,
      .owner = NULL,
      .name = "saber_imu",
    },
    &s_imu_daemon
  );
  if (ret != 0) {
    return ret;
  }

  RSCFLedStatusSetHealthy(RSCF_LED_STATUS_IMU, true);
  LOG_INF("imu uart initialized");
  return 0;
}

int RSCFImuUartInjectFrame(const uint8_t *data, size_t len)
{
  bool updated;

  if (!s_imu_ready) {
    return -ENODEV;
  }

  if ((data == NULL) || (len < RSCF_SABER_FRAME_MIN_SIZE)) {
    return -EINVAL;
  }

  if ((data[0] != 'A') || (data[1] != 'x') || (data[len - 1U] != 'm')) {
    return -EINVAL;
  }

  updated = rscf_imu_uart_decode(data, len);
  if (!updated) {
    return -ENODATA;
  }

  s_imu_sample_seq++;
  s_imu_latest.update_tick = k_uptime_get_32();
  s_imu_latest.sample_seq = s_imu_sample_seq;
  s_imu_has_frame = true;
  RSCFDaemonReload(s_imu_daemon);
  RSCFLedStatusBeat(RSCF_LED_STATUS_IMU);
  RSCFLedStatusSetHealthy(RSCF_LED_STATUS_IMU, true);
  return RSCFEventBusPublish(&s_imu_publisher, &s_imu_latest);
}

const struct rscf_saber_topic_data *RSCFImuUartLatest(void)
{
  if (!s_imu_has_frame) {
    return NULL;
  }

  return &s_imu_latest;
}

bool RSCFImuUartTryGetLatest(struct rscf_saber_topic_data *out)
{
  if ((!s_imu_has_frame) || (out == NULL)) {
    return false;
  }

  *out = s_imu_latest;
  return true;
}

bool RSCFImuUartReady(void)
{
  return s_imu_ready;
}

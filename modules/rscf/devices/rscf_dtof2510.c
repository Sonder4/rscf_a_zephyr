#include "rscf_dtof2510.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <rscf/rscf_daemon_service.h>
#include <rscf/rscf_event_bus.h>

#include "rscf_ext_uart_mux.h"

LOG_MODULE_REGISTER(rscf_dtof2510, LOG_LEVEL_INF);

#define RSCF_DTOF2510_NODE DT_NODELABEL(rscf_dtof2510_group)

static struct rscf_dtof2510_frame s_latest[RSCF_DTOF2510_MAX_CHANNELS];
static struct rscf_dtof2510_frame s_history[RSCF_DTOF2510_MAX_CHANNELS][RSCF_DTOF2510_HISTORY_DEPTH];
static uint8_t s_history_head[RSCF_DTOF2510_MAX_CHANNELS];
static uint32_t s_sample_seq[RSCF_DTOF2510_MAX_CHANNELS];
static struct rscf_event_bus_publisher s_publishers[RSCF_DTOF2510_MAX_CHANNELS];
static struct rscf_daemon_handle *s_daemons[RSCF_DTOF2510_MAX_CHANNELS];
static uint8_t s_channel_count;
static bool s_ready;

static const char *const s_topic_names[RSCF_DTOF2510_MAX_CHANNELS] = {
  "dtof_2510_ch0",
  "dtof_2510_ch1",
  "dtof_2510_ch2",
  "dtof_2510_ch3",
};

static void rscf_dtof2510_daemon_callback(void *owner, enum rscf_daemon_event event)
{
  uint32_t channel = (uint32_t)(uintptr_t)owner;

  if (event == RSCF_DAEMON_EVENT_OFFLINE_ENTER) {
    LOG_WRN("dtof channel %u offline", channel);
  } else if (event == RSCF_DAEMON_EVENT_ONLINE_RECOVER) {
    LOG_INF("dtof channel %u recovered", channel);
  }
}

static int rscf_dtof2510_parse_line(const char *line, struct rscf_dtof2510_frame *frame)
{
  char buffer[32];
  char *comma;

  if ((line == NULL) || (frame == NULL)) {
    return -EINVAL;
  }

  memset(buffer, 0, sizeof(buffer));
  strncpy(buffer, line, sizeof(buffer) - 1U);
  comma = strchr(buffer, ',');
  if (comma == NULL) {
    return -EINVAL;
  }

  *comma = '\0';
  frame->distance_mm = (uint16_t)strtoul(buffer, NULL, 10);
  frame->distance_raw_mm_f = (float)frame->distance_mm;
  frame->confidence = (uint16_t)strtoul(comma + 1, NULL, 10);
  frame->tick_ms = k_uptime_get_32();
  frame->valid = (frame->distance_mm >= 20U) && (frame->distance_mm <= 10000U) &&
                 (frame->confidence <= 1000U);
  return 0;
}

int RSCFDtof2510Init(void)
{
  int ret;

#if DT_NODE_HAS_STATUS(RSCF_DTOF2510_NODE, okay)
  s_channel_count = DT_PROP(RSCF_DTOF2510_NODE, channel_count);
  if (s_channel_count > RSCF_DTOF2510_MAX_CHANNELS) {
    s_channel_count = RSCF_DTOF2510_MAX_CHANNELS;
  }
#else
  s_channel_count = 0U;
#endif

  memset(s_latest, 0, sizeof(s_latest));
  memset(s_history, 0, sizeof(s_history));
  memset(s_history_head, 0, sizeof(s_history_head));
  memset(s_sample_seq, 0, sizeof(s_sample_seq));
  memset(s_publishers, 0, sizeof(s_publishers));
  memset(s_daemons, 0, sizeof(s_daemons));

  s_ready = (RSCFExtUartMuxUart() != NULL) && (s_channel_count > 0U);
  if (s_ready) {
    for (uint8_t channel = 0U; channel < s_channel_count; ++channel) {
      ret = RSCFEventBusPublisherRegister(
        s_topic_names[channel],
        sizeof(s_latest[channel]),
        &s_publishers[channel]
      );
      if (ret != 0) {
        return ret;
      }

      ret = RSCFDaemonRegister(
        &(const struct rscf_daemon_config){
          .reload_ticks = 100U,
          .init_ticks = 100U,
          .remind_ticks = 1000U,
          .callback = rscf_dtof2510_daemon_callback,
          .owner = (void *)(uintptr_t)channel,
          .name = s_topic_names[channel],
        },
        &s_daemons[channel]
      );
      if (ret != 0) {
        return ret;
      }
    }
  }

  LOG_INF("dtof2510 ready=%d channels=%u", s_ready ? 1 : 0, s_channel_count);
  return 0;
}

int RSCFDtof2510InjectFrame(uint8_t channel, const char *line)
{
  struct rscf_dtof2510_frame frame = {0};
  uint8_t head;
  int ret;

  if ((!s_ready) || (channel >= s_channel_count)) {
    return -EINVAL;
  }

  ret = rscf_dtof2510_parse_line(line, &frame);
  if (ret != 0) {
    return ret;
  }

  frame.sample_seq = ++s_sample_seq[channel];
  head = s_history_head[channel];
  s_history[channel][head] = frame;
  s_latest[channel] = frame;
  s_history_head[channel] = (uint8_t)((head + 1U) % RSCF_DTOF2510_HISTORY_DEPTH);
  RSCFDaemonReload(s_daemons[channel]);
  ret = RSCFEventBusPublish(&s_publishers[channel], &s_latest[channel]);
  if (ret != 0) {
    return ret;
  }

  return 0;
}

const struct rscf_dtof2510_frame *RSCFDtof2510Latest(uint8_t channel)
{
  if (channel >= s_channel_count) {
    return NULL;
  }

  return &s_latest[channel];
}

const char *RSCFDtof2510TopicName(uint8_t channel)
{
  if (channel >= s_channel_count) {
    return NULL;
  }

  return s_topic_names[channel];
}

bool RSCFDtof2510Ready(void)
{
  return s_ready;
}

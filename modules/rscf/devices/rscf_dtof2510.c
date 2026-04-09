#include "rscf_dtof2510.h"

#include <stdlib.h>
#include <string.h>

#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>

#include "rscf_ext_uart_mux.h"

LOG_MODULE_REGISTER(rscf_dtof2510, LOG_LEVEL_INF);

#define RSCF_DTOF2510_NODE DT_NODELABEL(rscf_dtof2510_group)

static struct rscf_dtof2510_frame s_latest[RSCF_DTOF2510_MAX_CHANNELS];
static struct rscf_dtof2510_frame s_history[RSCF_DTOF2510_MAX_CHANNELS][RSCF_DTOF2510_HISTORY_DEPTH];
static uint8_t s_history_head[RSCF_DTOF2510_MAX_CHANNELS];
static uint8_t s_channel_count;
static bool s_ready;

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
  frame->valid = (frame->distance_mm >= 20U) && (frame->distance_mm <= 10000U) &&
                 (frame->confidence <= 1000U);
  return 0;
}

int RSCFDtof2510Init(void)
{
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

  s_ready = (RSCFExtUartMuxUart() != NULL) && (s_channel_count > 0U);
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

  head = s_history_head[channel];
  s_history[channel][head] = frame;
  s_latest[channel] = frame;
  s_history_head[channel] = (uint8_t)((head + 1U) % RSCF_DTOF2510_HISTORY_DEPTH);
  return 0;
}

const struct rscf_dtof2510_frame *RSCFDtof2510Latest(uint8_t channel)
{
  if (channel >= s_channel_count) {
    return NULL;
  }

  return &s_latest[channel];
}

bool RSCFDtof2510Ready(void)
{
  return s_ready;
}

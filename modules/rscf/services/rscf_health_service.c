#include <zephyr/logging/log.h>

#include <rscf/rscf_health_service.h>

LOG_MODULE_REGISTER(rscf_health, LOG_LEVEL_INF);

static unsigned int s_beat_count;

int RSCFHealthServiceInit(void)
{
  s_beat_count = 0U;
  LOG_INF("health service baseline initialized");
  return 0;
}

void RSCFHealthServiceBeat(void)
{
  s_beat_count++;

  if ((s_beat_count % 500U) == 0U) {
    LOG_INF("health beat count=%u", s_beat_count);
  }
}

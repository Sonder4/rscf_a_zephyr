#include <zephyr/logging/log.h>

#include <rscf/rscf_event_bus.h>

LOG_MODULE_REGISTER(rscf_event_bus, LOG_LEVEL_INF);

int RSCFEventBusInit(void)
{
  LOG_INF("event bus baseline initialized");
  return 0;
}

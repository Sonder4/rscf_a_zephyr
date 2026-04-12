#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rscf_action_service, LOG_LEVEL_INF);

static bool s_action_service_ready;

int RSCFActionServiceInit(void)
{
  s_action_service_ready = true;
  return 0;
}

void RSCFActionServiceProcess(void)
{
  if (!s_action_service_ready) {
    return;
  }
}

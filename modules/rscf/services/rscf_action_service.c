#include "rscf_link_service.h"

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rscf_action_service, LOG_LEVEL_INF);

int RSCFActionServiceInit(void)
{
  return 0;
}

void RSCFActionServiceProcess(struct rscf_link_runtime *runtime)
{
  if ((runtime == NULL) || !runtime->ready) {
    return;
  }

  if (runtime->handled_events >= runtime->last_event_sequence) {
    return;
  }

  runtime->handled_events++;
}

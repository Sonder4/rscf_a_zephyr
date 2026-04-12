#include "rscf_link_service.h"

#include <errno.h>

int RSCFServiceRouterInit(void)
{
  return 0;
}

int RSCFServiceRouterDispatch(struct rscf_link_runtime *runtime)
{
  struct rscf_link_runtime_event event;

  if ((runtime == NULL) || !runtime->ready) {
    return -EINVAL;
  }

  if (k_msgq_get(&runtime->event_queue, &event, K_NO_WAIT) != 0) {
    return 0;
  }

  runtime->last_event_type = event.type;
  runtime->last_event_sequence = event.sequence;
  return 0;
}

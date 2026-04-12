#include "rscf_link_service.h"

#include <errno.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rscf_service_router, LOG_LEVEL_INF);

static struct k_msgq *s_service_msgq;

int RSCFServiceRouterInit(struct k_msgq *service_msgq)
{
  if (service_msgq == NULL) {
    return -EINVAL;
  }

  s_service_msgq = service_msgq;
  return 0;
}

void RSCFServiceRouterDispatch(struct k_msgq *service_msgq)
{
  struct rscf_link_service_request request;
  struct k_msgq *target_msgq = service_msgq != NULL ? service_msgq : s_service_msgq;

  if (target_msgq == NULL) {
    return;
  }

  while (k_msgq_get(target_msgq, &request, K_NO_WAIT) == 0) {
    LOG_DBG("service request skeleton endpoint=0x%04x len=%u",
            request.endpoint_id,
            request.payload_len);
  }
}

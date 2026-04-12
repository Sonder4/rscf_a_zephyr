#include "rscf_link_service.h"

#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(rscf_link_service, LOG_LEVEL_INF);

static struct rscf_link_runtime s_runtime;

static void rscf_link_service_poll_handler(struct k_work *work)
{
  ARG_UNUSED(work);
  struct rscf_link_runtime_event event = {
    .type = RSCF_LINK_RUNTIME_EVENT_POLL,
    .reserved = 0U,
    .length = 0U,
    .sequence = s_runtime.scheduled_events + 1U,
  };

  if (!s_runtime.ready) {
    return;
  }

  s_runtime.scheduled_events = event.sequence;
  (void)k_msgq_put(&s_runtime.event_queue, &event, K_NO_WAIT);
}

static void rscf_link_service_runtime_init(struct rscf_link_runtime *runtime)
{
  memset(runtime, 0, sizeof(*runtime));
  ring_buf_init(&runtime->rx_ring, sizeof(runtime->rx_storage), runtime->rx_storage);
  k_msgq_init(&runtime->event_queue,
              (char *)runtime->event_storage,
              sizeof(runtime->event_storage[0]),
              ARRAY_SIZE(runtime->event_storage));
  k_work_init_delayable(&runtime->poll_work, rscf_link_service_poll_handler);
  runtime->ready = true;
}

int RSCFLinkServiceInit(void)
{
  int ret;

  if (s_runtime.ready) {
    return 0;
  }

  rscf_link_service_runtime_init(&s_runtime);

  ret = RSCFServiceRouterInit();
  if (ret != 0) {
    s_runtime.ready = false;
    return ret;
  }

  ret = RSCFActionServiceInit();
  if (ret != 0) {
    s_runtime.ready = false;
    return ret;
  }

  LOG_INF("link runtime skeleton initialized");
  return 0;
}

void RSCFLinkServiceProcess(void)
{
  if (!s_runtime.ready) {
    return;
  }

  (void)k_work_reschedule(&s_runtime.poll_work, K_NO_WAIT);
  (void)RSCFServiceRouterDispatch(&s_runtime);
  RSCFActionServiceProcess(&s_runtime);
}

struct rscf_link_runtime *RSCFLinkServiceGetRuntime(void)
{
  return s_runtime.ready ? &s_runtime : NULL;
}

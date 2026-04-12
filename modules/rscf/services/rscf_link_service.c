#include "rscf_link_service.h"

#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rscf_link_service, LOG_LEVEL_INF);

int RSCFServiceRouterInit(struct k_msgq *service_msgq);
void RSCFServiceRouterDispatch(struct k_msgq *service_msgq);
int RSCFActionServiceInit(void);
void RSCFActionServiceProcess(void);

static struct rscf_link_runtime s_link_runtime;

static void rscf_link_heartbeat_work_handler(struct k_work *work)
{
  ARG_UNUSED(work);
}

static void rscf_link_timeout_work_handler(struct k_work *work)
{
  ARG_UNUSED(work);
}

static void rscf_link_dispatch_frame(struct rscf_link_runtime *runtime,
                                     const struct rscf_link_frame *frame)
{
  struct rscf_link_service_request request;

  if ((runtime == NULL) || (frame == NULL)) {
    return;
  }

  if (frame->endpoint_class != RSCF_LINK_ENDPOINT_SERVICE_REQ) {
    return;
  }

  memset(&request, 0, sizeof(request));
  request.endpoint_id = frame->endpoint_id;
  request.payload_len = frame->payload_len;
  memcpy(request.payload, frame->payload, frame->payload_len);
  (void)k_msgq_put(&runtime->service_msgq, &request, K_NO_WAIT);
}

static void rscf_link_process_ring(struct rscf_link_runtime *runtime, struct ring_buf *ring)
{
  struct rscf_link_frame frame;
  uint8_t buffer[RSCF_LINK_FRAME_PAYLOAD_MAX];
  uint32_t read_len;

  if ((runtime == NULL) || (ring == NULL)) {
    return;
  }

  read_len = ring_buf_get(ring, buffer, sizeof(buffer));
  if (read_len == 0U) {
    return;
  }

  memset(&frame, 0, sizeof(frame));
  frame.endpoint_class = RSCF_LINK_ENDPOINT_CUSTOM;
  frame.payload_len = (uint16_t)read_len;
  memcpy(frame.payload, buffer, read_len);
  rscf_link_dispatch_frame(runtime, &frame);
}

static int rscf_link_push_bytes(struct ring_buf *ring,
                                uint32_t *drop_count,
                                const uint8_t *data,
                                size_t len)
{
  uint32_t written;

  if ((ring == NULL) || (drop_count == NULL) || (data == NULL) || (len == 0U)) {
    return -EINVAL;
  }

  written = ring_buf_put(ring, data, (uint32_t)len);
  if (written != len) {
    *drop_count += (uint32_t)(len - written);
    return -ENOSPC;
  }

  return 0;
}

int RSCFLinkServiceInit(void)
{
  memset(&s_link_runtime, 0, sizeof(s_link_runtime));
  ring_buf_init(&s_link_runtime.host_rx_ring,
                sizeof(s_link_runtime.host_rx_storage),
                s_link_runtime.host_rx_storage);
  ring_buf_init(&s_link_runtime.peer_rx_ring,
                sizeof(s_link_runtime.peer_rx_storage),
                s_link_runtime.peer_rx_storage);
  k_msgq_init(&s_link_runtime.service_msgq,
              s_link_runtime.service_msgq_storage,
              sizeof(struct rscf_link_service_request),
              RSCF_LINK_SERVICE_QUEUE_DEPTH);
  k_work_init_delayable(&s_link_runtime.heartbeat_work, rscf_link_heartbeat_work_handler);
  k_work_init_delayable(&s_link_runtime.timeout_work, rscf_link_timeout_work_handler);

  if (RSCFServiceRouterInit(&s_link_runtime.service_msgq) != 0) {
    return -EINVAL;
  }
  if (RSCFActionServiceInit() != 0) {
    return -EINVAL;
  }

  s_link_runtime.initialized = true;
  LOG_INF("link runtime skeleton initialized");
  return 0;
}

void RSCFLinkServiceProcess(void)
{
  if (!s_link_runtime.initialized) {
    return;
  }

  rscf_link_process_ring(&s_link_runtime, &s_link_runtime.host_rx_ring);
  rscf_link_process_ring(&s_link_runtime, &s_link_runtime.peer_rx_ring);
  RSCFServiceRouterDispatch(&s_link_runtime.service_msgq);
  RSCFActionServiceProcess();
}

bool RSCFLinkServiceReady(void)
{
  return s_link_runtime.initialized;
}

int RSCFLinkServicePushHostBytes(const uint8_t *data, size_t len)
{
  return rscf_link_push_bytes(&s_link_runtime.host_rx_ring,
                              &s_link_runtime.host_rx_drop_count,
                              data,
                              len);
}

int RSCFLinkServicePushPeerBytes(const uint8_t *data, size_t len)
{
  return rscf_link_push_bytes(&s_link_runtime.peer_rx_ring,
                              &s_link_runtime.peer_rx_drop_count,
                              data,
                              len);
}

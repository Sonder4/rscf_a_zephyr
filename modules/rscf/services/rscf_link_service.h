#ifndef RSCF_LINK_SERVICE_H_
#define RSCF_LINK_SERVICE_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RSCF_LINK_RING_CAPACITY 256U
#define RSCF_LINK_SERVICE_QUEUE_DEPTH 8U
#define RSCF_LINK_FRAME_PAYLOAD_MAX 64U

enum rscf_link_endpoint_class {
  RSCF_LINK_ENDPOINT_TOPIC = 0,
  RSCF_LINK_ENDPOINT_SERVICE_REQ = 1,
  RSCF_LINK_ENDPOINT_ACTION = 2,
  RSCF_LINK_ENDPOINT_CUSTOM = 3,
};

struct rscf_link_frame {
  uint8_t endpoint_class;
  uint16_t endpoint_id;
  uint16_t payload_len;
  uint8_t payload[RSCF_LINK_FRAME_PAYLOAD_MAX];
};

struct rscf_link_service_request {
  uint16_t endpoint_id;
  uint16_t payload_len;
  uint8_t payload[RSCF_LINK_FRAME_PAYLOAD_MAX];
};

struct rscf_link_runtime {
  struct ring_buf host_rx_ring;
  struct ring_buf peer_rx_ring;
  struct k_work_delayable heartbeat_work;
  struct k_work_delayable timeout_work;
  struct k_msgq service_msgq;
  uint8_t host_rx_storage[RSCF_LINK_RING_CAPACITY];
  uint8_t peer_rx_storage[RSCF_LINK_RING_CAPACITY];
  char service_msgq_storage[
    sizeof(struct rscf_link_service_request) * RSCF_LINK_SERVICE_QUEUE_DEPTH
  ];
  uint32_t host_rx_drop_count;
  uint32_t peer_rx_drop_count;
  bool initialized;
};

int RSCFLinkServiceInit(void);
void RSCFLinkServiceProcess(void);
bool RSCFLinkServiceReady(void);
int RSCFLinkServicePushHostBytes(const uint8_t *data, size_t len);
int RSCFLinkServicePushPeerBytes(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* RSCF_LINK_SERVICE_H_ */

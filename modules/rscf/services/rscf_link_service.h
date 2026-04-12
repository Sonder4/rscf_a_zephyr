#ifndef RSCF_LINK_SERVICE_H_
#define RSCF_LINK_SERVICE_H_

#include <stdbool.h>
#include <stdint.h>

#include <zephyr/kernel.h>
#include <zephyr/sys/ring_buffer.h>

#define RSCF_LINK_SERVICE_RING_BUFFER_SIZE 256U
#define RSCF_LINK_SERVICE_MSGQ_DEPTH 4U

enum rscf_link_runtime_event_type {
  RSCF_LINK_RUNTIME_EVENT_NONE = 0,
  RSCF_LINK_RUNTIME_EVENT_POLL = 1,
};

struct rscf_link_runtime_event {
  uint8_t type;
  uint8_t reserved;
  uint16_t length;
  uint32_t sequence;
};

struct rscf_link_runtime {
  struct ring_buf rx_ring;
  struct k_work_delayable poll_work;
  struct k_msgq event_queue;
  uint8_t rx_storage[RSCF_LINK_SERVICE_RING_BUFFER_SIZE];
  struct rscf_link_runtime_event event_storage[RSCF_LINK_SERVICE_MSGQ_DEPTH];
  uint32_t scheduled_events;
  uint32_t handled_events;
  uint32_t last_event_sequence;
  uint8_t last_event_type;
  bool ready;
};

int RSCFLinkServiceInit(void);
void RSCFLinkServiceProcess(void);
struct rscf_link_runtime *RSCFLinkServiceGetRuntime(void);

int RSCFServiceRouterInit(void);
int RSCFServiceRouterDispatch(struct rscf_link_runtime *runtime);

int RSCFActionServiceInit(void);
void RSCFActionServiceProcess(struct rscf_link_runtime *runtime);

#endif /* RSCF_LINK_SERVICE_H_ */

#ifndef RSCF_EVENT_BUS_H_
#define RSCF_EVENT_BUS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define RSCF_EVENT_BUS_MAX_TOPIC_NAME 32U
#define RSCF_EVENT_BUS_MAX_TOPICS 16U
#define RSCF_EVENT_BUS_MAX_SUBSCRIBERS 24U
#define RSCF_EVENT_BUS_MAX_PAYLOAD 64U

struct rscf_event_bus_publisher {
  uint8_t topic_index;
  uint8_t valid;
  uint16_t data_size;
};

struct rscf_event_bus_subscriber {
  uint8_t topic_index;
  uint8_t valid;
  uint16_t data_size;
  uint32_t sequence;
  uint32_t last_consumed_sequence;
  uint8_t payload[RSCF_EVENT_BUS_MAX_PAYLOAD];
};

int RSCFEventBusInit(void);
int RSCFEventBusPublisherRegister(
  const char *topic_name,
  size_t data_size,
  struct rscf_event_bus_publisher *publisher
);
int RSCFEventBusSubscriberRegister(
  const char *topic_name,
  size_t data_size,
  struct rscf_event_bus_subscriber *subscriber
);
int RSCFEventBusPublish(
  const struct rscf_event_bus_publisher *publisher,
  const void *payload
);
int RSCFEventBusConsume(
  struct rscf_event_bus_subscriber *subscriber,
  void *payload,
  bool *updated
);
bool RSCFEventBusSubscriberHasUpdate(
  const struct rscf_event_bus_subscriber *subscriber
);

#endif /* RSCF_EVENT_BUS_H_ */

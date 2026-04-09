#include <errno.h>
#include <string.h>

#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include <rscf/rscf_event_bus.h>

LOG_MODULE_REGISTER(rscf_event_bus, LOG_LEVEL_INF);

struct rscf_event_bus_topic {
  char name[RSCF_EVENT_BUS_MAX_TOPIC_NAME + 1U];
  uint16_t data_size;
  uint8_t ready;
  uint32_t publish_count;
};

static struct rscf_event_bus_topic s_topics[RSCF_EVENT_BUS_MAX_TOPICS];
static struct rscf_event_bus_subscriber *s_subscribers[RSCF_EVENT_BUS_MAX_SUBSCRIBERS];
static struct k_spinlock s_event_bus_lock;
static bool s_initialized;

static int rscf_event_bus_find_topic(const char *topic_name)
{
  for (size_t i = 0; i < ARRAY_SIZE(s_topics); ++i) {
    if (s_topics[i].ready == 0U) {
      continue;
    }

    if (strncmp(s_topics[i].name, topic_name, sizeof(s_topics[i].name)) == 0) {
      return (int)i;
    }
  }

  return -ENOENT;
}

static int rscf_event_bus_get_or_create_topic(const char *topic_name, size_t data_size)
{
  int topic_index;

  topic_index = rscf_event_bus_find_topic(topic_name);
  if (topic_index >= 0) {
    if (s_topics[topic_index].data_size != data_size) {
      return -EINVAL;
    }

    return topic_index;
  }

  for (size_t i = 0; i < ARRAY_SIZE(s_topics); ++i) {
    if (s_topics[i].ready != 0U) {
      continue;
    }

    memset(&s_topics[i], 0, sizeof(s_topics[i]));
    strncpy(s_topics[i].name, topic_name, RSCF_EVENT_BUS_MAX_TOPIC_NAME);
    s_topics[i].data_size = (uint16_t)data_size;
    s_topics[i].ready = 1U;
    return (int)i;
  }

  return -ENOSPC;
}

int RSCFEventBusInit(void)
{
  k_spinlock_key_t key = k_spin_lock(&s_event_bus_lock);

  memset(s_topics, 0, sizeof(s_topics));
  memset(s_subscribers, 0, sizeof(s_subscribers));
  s_initialized = true;
  k_spin_unlock(&s_event_bus_lock, key);

  LOG_INF("event bus initialized");
  return 0;
}

int RSCFEventBusPublisherRegister(
  const char *topic_name,
  size_t data_size,
  struct rscf_event_bus_publisher *publisher
)
{
  k_spinlock_key_t key;
  int topic_index;

  if ((!s_initialized) || (topic_name == NULL) || (publisher == NULL)) {
    return -EINVAL;
  }

  if ((data_size == 0U) || (data_size > RSCF_EVENT_BUS_MAX_PAYLOAD) ||
      (strnlen(topic_name, RSCF_EVENT_BUS_MAX_TOPIC_NAME + 1U) > RSCF_EVENT_BUS_MAX_TOPIC_NAME)) {
    return -EINVAL;
  }

  key = k_spin_lock(&s_event_bus_lock);
  topic_index = rscf_event_bus_get_or_create_topic(topic_name, data_size);
  if (topic_index >= 0) {
    publisher->topic_index = (uint8_t)topic_index;
    publisher->data_size = (uint16_t)data_size;
    publisher->valid = 1U;
  }
  k_spin_unlock(&s_event_bus_lock, key);
  return topic_index < 0 ? topic_index : 0;
}

int RSCFEventBusSubscriberRegister(
  const char *topic_name,
  size_t data_size,
  struct rscf_event_bus_subscriber *subscriber
)
{
  k_spinlock_key_t key;
  int topic_index;
  int free_slot = -1;

  if ((!s_initialized) || (topic_name == NULL) || (subscriber == NULL)) {
    return -EINVAL;
  }

  if ((data_size == 0U) || (data_size > RSCF_EVENT_BUS_MAX_PAYLOAD) ||
      (strnlen(topic_name, RSCF_EVENT_BUS_MAX_TOPIC_NAME + 1U) > RSCF_EVENT_BUS_MAX_TOPIC_NAME)) {
    return -EINVAL;
  }

  key = k_spin_lock(&s_event_bus_lock);
  topic_index = rscf_event_bus_get_or_create_topic(topic_name, data_size);
  if (topic_index < 0) {
    k_spin_unlock(&s_event_bus_lock, key);
    return topic_index;
  }

  for (size_t i = 0; i < ARRAY_SIZE(s_subscribers); ++i) {
    if (s_subscribers[i] == subscriber) {
      free_slot = (int)i;
      break;
    }

    if ((free_slot < 0) && (s_subscribers[i] == NULL)) {
      free_slot = (int)i;
    }
  }

  if (free_slot < 0) {
    k_spin_unlock(&s_event_bus_lock, key);
    return -ENOSPC;
  }

  memset(subscriber, 0, sizeof(*subscriber));
  subscriber->topic_index = (uint8_t)topic_index;
  subscriber->data_size = (uint16_t)data_size;
  subscriber->valid = 1U;
  s_subscribers[free_slot] = subscriber;
  k_spin_unlock(&s_event_bus_lock, key);
  return 0;
}

int RSCFEventBusPublish(
  const struct rscf_event_bus_publisher *publisher,
  const void *payload
)
{
  k_spinlock_key_t key;
  struct rscf_event_bus_topic *topic;
  uint32_t sequence;

  if ((!s_initialized) || (publisher == NULL) || (payload == NULL) ||
      (publisher->valid == 0U) || (publisher->topic_index >= ARRAY_SIZE(s_topics))) {
    return -EINVAL;
  }

  key = k_spin_lock(&s_event_bus_lock);
  topic = &s_topics[publisher->topic_index];
  if ((topic->ready == 0U) || (topic->data_size != publisher->data_size)) {
    k_spin_unlock(&s_event_bus_lock, key);
    return -EINVAL;
  }

  topic->publish_count++;
  sequence = topic->publish_count;

  for (size_t i = 0; i < ARRAY_SIZE(s_subscribers); ++i) {
    struct rscf_event_bus_subscriber *subscriber = s_subscribers[i];

    if ((subscriber == NULL) || (subscriber->valid == 0U) ||
        (subscriber->topic_index != publisher->topic_index)) {
      continue;
    }

    memcpy(subscriber->payload, payload, publisher->data_size);
    subscriber->sequence = sequence;
  }

  k_spin_unlock(&s_event_bus_lock, key);
  return 0;
}

int RSCFEventBusConsume(
  struct rscf_event_bus_subscriber *subscriber,
  void *payload,
  bool *updated
)
{
  k_spinlock_key_t key;
  bool has_update;

  if ((subscriber == NULL) || (payload == NULL) || (updated == NULL) ||
      (subscriber->valid == 0U)) {
    return -EINVAL;
  }

  key = k_spin_lock(&s_event_bus_lock);
  has_update = subscriber->sequence != subscriber->last_consumed_sequence;
  *updated = has_update;
  if (has_update) {
    memcpy(payload, subscriber->payload, subscriber->data_size);
    subscriber->last_consumed_sequence = subscriber->sequence;
  }
  k_spin_unlock(&s_event_bus_lock, key);
  return 0;
}

bool RSCFEventBusSubscriberHasUpdate(
  const struct rscf_event_bus_subscriber *subscriber
)
{
  k_spinlock_key_t key;
  bool has_update;

  if ((subscriber == NULL) || (subscriber->valid == 0U)) {
    return false;
  }

  key = k_spin_lock(&s_event_bus_lock);
  has_update = subscriber->sequence != subscriber->last_consumed_sequence;
  k_spin_unlock(&s_event_bus_lock, key);
  return has_update;
}

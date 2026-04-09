/*******************************************************************************

                      Copyright (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  File Name    : rscf_daemon_service.c
  Version      : V20260410.1
  Author       : Codex
  Created Date : 2026-04-10
  Description  : Zephyr 守护服务实现
  Supplement   : 用 delayable work 周期更新模块在线状态

*******************************************************************************/

#include <errno.h>
#include <string.h>

#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

#include <rscf/rscf_daemon_service.h>
#include "rscf_led_status.h"

LOG_MODULE_REGISTER(rscf_daemon_service, LOG_LEVEL_INF);

#define RSCF_DAEMON_MAX_WATCHDOGS 32U
#define RSCF_DAEMON_PERIOD_MS 10U
#define RSCF_DAEMON_DEFAULT_RELOAD_TICKS 100U
#define RSCF_DAEMON_MAX_NAME_LEN 24U

struct rscf_daemon_handle {
  uint16_t reload_ticks;
  uint16_t remaining_ticks;
  uint16_t remind_ticks;
  uint16_t remind_countdown;
  rscf_daemon_callback_t callback;
  void *owner;
  char name[RSCF_DAEMON_MAX_NAME_LEN];
  uint8_t in_use;
  uint8_t online;
  uint8_t pending_event;
};

struct rscf_daemon_callback_record {
  struct rscf_daemon_handle *handle;
  enum rscf_daemon_event event;
};

static struct rscf_daemon_handle s_daemons[RSCF_DAEMON_MAX_WATCHDOGS];
static struct k_spinlock s_daemon_lock;
static struct k_work_delayable s_daemon_work;
static bool s_service_started;

static void rscf_daemon_service_reschedule(void)
{
  (void)k_work_reschedule(&s_daemon_work, K_MSEC(RSCF_DAEMON_PERIOD_MS));
}

static void rscf_daemon_service_emit(
  struct rscf_daemon_callback_record *records,
  size_t record_count
)
{
  for (size_t i = 0; i < record_count; ++i) {
    if ((records[i].handle != NULL) && (records[i].handle->callback != NULL)) {
      records[i].handle->callback(records[i].handle->owner, records[i].event);
    }
  }
}

static void rscf_daemon_service_work_handler(struct k_work *work)
{
  struct rscf_daemon_callback_record records[RSCF_DAEMON_MAX_WATCHDOGS];
  k_spinlock_key_t key;
  size_t record_count = 0U;
  bool all_online = true;

  ARG_UNUSED(work);
  memset(records, 0, sizeof(records));

  key = k_spin_lock(&s_daemon_lock);
  for (size_t i = 0; i < ARRAY_SIZE(s_daemons); ++i) {
    struct rscf_daemon_handle *handle = &s_daemons[i];

    if (handle->in_use == 0U) {
      continue;
    }

    if (handle->remaining_ticks > 0U) {
      handle->remaining_ticks--;
      if ((handle->remaining_ticks == 0U) && (handle->online != 0U)) {
        handle->online = 0U;
        handle->remind_countdown = handle->remind_ticks;
        handle->pending_event = RSCF_DAEMON_EVENT_OFFLINE_ENTER;
      }
    } else if ((handle->online == 0U) && (handle->remind_ticks > 0U)) {
      if (handle->remind_countdown > 0U) {
        handle->remind_countdown--;
      }

      if (handle->remind_countdown == 0U) {
        handle->pending_event = RSCF_DAEMON_EVENT_OFFLINE_REMIND;
        handle->remind_countdown = handle->remind_ticks;
      }
    }

    if (handle->online == 0U) {
      all_online = false;
    }

    if ((handle->pending_event != RSCF_DAEMON_EVENT_NONE) &&
        (record_count < ARRAY_SIZE(records))) {
      records[record_count].handle = handle;
      records[record_count].event = (enum rscf_daemon_event)handle->pending_event;
      handle->pending_event = RSCF_DAEMON_EVENT_NONE;
      record_count++;
    }
  }
  k_spin_unlock(&s_daemon_lock, key);

#if defined(CONFIG_RSCF_LED_STATUS)
  RSCFLedStatusSetHealthy(RSCF_LED_STATUS_DAEMON, all_online);
#endif
  rscf_daemon_service_emit(records, record_count);
  rscf_daemon_service_reschedule();
}

int RSCFDaemonServiceInit(void)
{
  if (s_service_started) {
    return 0;
  }

  memset(s_daemons, 0, sizeof(s_daemons));
  k_work_init_delayable(&s_daemon_work, rscf_daemon_service_work_handler);
  s_service_started = true;
  rscf_daemon_service_reschedule();
  LOG_INF("daemon service initialized");
  return 0;
}

int RSCFDaemonRegister(
  const struct rscf_daemon_config *config,
  struct rscf_daemon_handle **handle
)
{
  k_spinlock_key_t key;

  if ((!s_service_started) || (config == NULL) || (handle == NULL)) {
    return -EINVAL;
  }

  key = k_spin_lock(&s_daemon_lock);
  for (size_t i = 0; i < ARRAY_SIZE(s_daemons); ++i) {
    struct rscf_daemon_handle *slot = &s_daemons[i];

    if (slot->in_use != 0U) {
      continue;
    }

    memset(slot, 0, sizeof(*slot));
    slot->reload_ticks = config->reload_ticks == 0U ? RSCF_DAEMON_DEFAULT_RELOAD_TICKS
                                                    : config->reload_ticks;
    slot->remaining_ticks = config->init_ticks == 0U ? slot->reload_ticks
                                                     : config->init_ticks;
    slot->remind_ticks = config->remind_ticks;
    slot->remind_countdown = slot->remind_ticks;
    slot->callback = config->callback;
    slot->owner = config->owner;
    slot->online = slot->remaining_ticks > 0U ? 1U : 0U;
    slot->in_use = 1U;
    if (config->name != NULL) {
      strncpy(slot->name, config->name, sizeof(slot->name) - 1U);
    }
    *handle = slot;
    k_spin_unlock(&s_daemon_lock, key);
    return 0;
  }
  k_spin_unlock(&s_daemon_lock, key);

  return -ENOSPC;
}

void RSCFDaemonReload(struct rscf_daemon_handle *handle)
{
  k_spinlock_key_t key;

  if ((handle == NULL) || (handle->in_use == 0U)) {
    return;
  }

  key = k_spin_lock(&s_daemon_lock);
  if (handle->online == 0U) {
    handle->online = 1U;
    handle->pending_event = RSCF_DAEMON_EVENT_ONLINE_RECOVER;
  }
  handle->remaining_ticks = handle->reload_ticks;
  handle->remind_countdown = handle->remind_ticks;
  k_spin_unlock(&s_daemon_lock, key);
}

bool RSCFDaemonIsOnline(const struct rscf_daemon_handle *handle)
{
  k_spinlock_key_t key;
  bool is_online;

  if ((handle == NULL) || (handle->in_use == 0U)) {
    return false;
  }

  key = k_spin_lock(&s_daemon_lock);
  is_online = handle->online != 0U;
  k_spin_unlock(&s_daemon_lock, key);
  return is_online;
}

uint16_t RSCFDaemonRemainingTicks(const struct rscf_daemon_handle *handle)
{
  k_spinlock_key_t key;
  uint16_t remaining_ticks;

  if ((handle == NULL) || (handle->in_use == 0U)) {
    return 0U;
  }

  key = k_spin_lock(&s_daemon_lock);
  remaining_ticks = handle->remaining_ticks;
  k_spin_unlock(&s_daemon_lock, key);
  return remaining_ticks;
}

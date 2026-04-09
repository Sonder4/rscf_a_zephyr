/*******************************************************************************

                      Copyright (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  File Name    : rscf_daemon_service.h
  Version      : V20260410.1
  Author       : Codex
  Created Date : 2026-04-10
  Description  : Zephyr 守护服务对外接口
  Supplement   : 提供模块在线检测、喂狗与离线回调能力

*******************************************************************************/

#ifndef RSCF_DAEMON_SERVICE_H_
#define RSCF_DAEMON_SERVICE_H_

#include <stdbool.h>
#include <stdint.h>

enum rscf_daemon_event {
  RSCF_DAEMON_EVENT_NONE = 0,
  RSCF_DAEMON_EVENT_OFFLINE_ENTER,
  RSCF_DAEMON_EVENT_OFFLINE_REMIND,
  RSCF_DAEMON_EVENT_ONLINE_RECOVER,
};

typedef void (*rscf_daemon_callback_t)(void *owner, enum rscf_daemon_event event);

struct rscf_daemon_handle;

struct rscf_daemon_config {
  uint16_t reload_ticks;
  uint16_t init_ticks;
  uint16_t remind_ticks;
  rscf_daemon_callback_t callback;
  void *owner;
  const char *name;
};

int RSCFDaemonServiceInit(void);
int RSCFDaemonRegister(
  const struct rscf_daemon_config *config,
  struct rscf_daemon_handle **handle
);
void RSCFDaemonReload(struct rscf_daemon_handle *handle);
bool RSCFDaemonIsOnline(const struct rscf_daemon_handle *handle);
uint16_t RSCFDaemonRemainingTicks(const struct rscf_daemon_handle *handle);

#endif /* RSCF_DAEMON_SERVICE_H_ */

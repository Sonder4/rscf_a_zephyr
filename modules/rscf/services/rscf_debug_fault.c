/*******************************************************************************

                      Copyright (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  File Name    : rscf_debug_fault.c
  Version      : V20260410.1
  Author       : Codex
  Created Date : 2026-04-10
  Description  : Zephyr 调试故障注入模块实现
  Supplement   : 通过全局场景值触发日志故障或确定性异常

*******************************************************************************/

#include <zephyr/logging/log.h>

#include <rscf/rscf_debug_fault.h>
#include "rscf_led_status.h"

LOG_MODULE_REGISTER(rscf_debug_fault, LOG_LEVEL_INF);

#if defined(CONFIG_RSCF_DEBUG_FAULT)
volatile uint32_t g_rscf_debug_fault_scenario;
static uint32_t s_last_triggered_scenario;
static bool s_fault_armed;

static void rscf_debug_fault_trigger_hardfault(void)
{
  volatile uint32_t *fault_addr = (volatile uint32_t *)0x00000001UL;

  *fault_addr = 0xDEADBEEFUL;
  for (;;) {
  }
}

__attribute__((noreturn)) static void rscf_debug_fault_trigger_usagefault(void)
{
#if defined(__arm__) || defined(__thumb__)
  __asm volatile("udf #0");
#else
  __builtin_trap();
#endif

  for (;;) {
  }
}

int RSCFDebugFaultInit(void)
{
  g_rscf_debug_fault_scenario = RSCF_DEBUG_FAULT_SCENARIO_NONE;
  s_last_triggered_scenario = RSCF_DEBUG_FAULT_SCENARIO_NONE;
  s_fault_armed = false;
  LOG_INF("debug fault service ready");
  return 0;
}

void RSCFDebugFaultPoll(void)
{
  uint32_t scenario = g_rscf_debug_fault_scenario;

  if (scenario == RSCF_DEBUG_FAULT_SCENARIO_NONE) {
    s_fault_armed = false;
    return;
  }

  if (s_fault_armed && (scenario == s_last_triggered_scenario)) {
    return;
  }

  s_fault_armed = true;
  s_last_triggered_scenario = scenario;
  g_rscf_debug_fault_scenario = RSCF_DEBUG_FAULT_SCENARIO_NONE;
  RSCFLedStatusSetHealthy(RSCF_LED_STATUS_FAULT, false);

  switch (scenario) {
    case RSCF_DEBUG_FAULT_SCENARIO_IMU_COMM_FAULT:
      LOG_WRN("debug fault scenario=1 imu_comm_fault");
      break;

    case RSCF_DEBUG_FAULT_SCENARIO_FLASH_PARAM_FAULT:
      LOG_ERR("debug fault scenario=2 flash_param_fault");
      break;

    case RSCF_DEBUG_FAULT_SCENARIO_HARDFAULT_BAD_PTR:
      LOG_ERR("debug fault scenario=3 hardfault_bad_ptr");
      rscf_debug_fault_trigger_hardfault();
      break;

    case RSCF_DEBUG_FAULT_SCENARIO_USAGEFAULT_UDF:
      LOG_ERR("debug fault scenario=4 usagefault_udf");
      rscf_debug_fault_trigger_usagefault();
      break;

    default:
      LOG_WRN("debug fault scenario unsupported=%u", scenario);
      break;
  }
}
#else
int RSCFDebugFaultInit(void)
{
  return 0;
}

void RSCFDebugFaultPoll(void)
{
}
#endif

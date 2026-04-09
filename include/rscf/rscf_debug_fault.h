/*******************************************************************************

                      Copyright (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  File Name    : rscf_debug_fault.h
  Version      : V20260410.1
  Author       : Codex
  Created Date : 2026-04-10
  Description  : Zephyr 调试故障注入模块接口
  Supplement   : 默认关闭，仅在调试构建中启用

*******************************************************************************/

#ifndef RSCF_DEBUG_FAULT_H_
#define RSCF_DEBUG_FAULT_H_

#include <stdint.h>

enum rscf_debug_fault_scenario {
  RSCF_DEBUG_FAULT_SCENARIO_NONE = 0,
  RSCF_DEBUG_FAULT_SCENARIO_IMU_COMM_FAULT = 1,
  RSCF_DEBUG_FAULT_SCENARIO_FLASH_PARAM_FAULT = 2,
  RSCF_DEBUG_FAULT_SCENARIO_HARDFAULT_BAD_PTR = 3,
  RSCF_DEBUG_FAULT_SCENARIO_USAGEFAULT_UDF = 4,
};

#if defined(CONFIG_RSCF_DEBUG_FAULT)
extern volatile uint32_t g_rscf_debug_fault_scenario;
#endif

int RSCFDebugFaultInit(void);
void RSCFDebugFaultPoll(void);

#endif /* RSCF_DEBUG_FAULT_H_ */

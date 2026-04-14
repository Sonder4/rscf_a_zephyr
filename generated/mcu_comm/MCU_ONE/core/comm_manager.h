/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  文 件 名: comm_manager.h
  版 本 号: 1.0.0
  作    者: Xuan
  生成日期: 2026-02-11
  功能描述: Zephyr 通信管理模块头文件
  补    充: 协议版本 2.0.0

*******************************************************************************/

#ifndef __COMM_MANAGER_H
#define __COMM_MANAGER_H

#include <stdbool.h>
#include <stdint.h>

#include "protocol.h"
#include "transport_interface.h"

int Comm_Init(Transport_interface_t* transport);
Transport_interface_t* Comm_GetTransport(void);
Transport_interface_t* Comm_ResolveTransportOrDefault(Transport_interface_t* transport);
void Comm_OnTransportResolved(Transport_interface_t* transport);
void Comm_RegisterCompatEndpoint(uint8_t pid, uint16_t endpoint_id);
void Comm_RegisterCompatEndpoints(void);
void Comm_OnCompatEndpointResolved(uint8_t pid, uint16_t endpoint_id);
uint16_t Comm_CompatMapEndpoint(uint8_t pid);
void Comm_Send(uint8_t mid, uint8_t pid, const uint8_t* data, uint8_t len);
void Comm_Batch_Begin(void);
void Comm_Batch_End(void);
void Comm_ProcessRx(void);
void Comm_Process(void);
bool Comm_IsConnected(void);
bool Comm_HasRxOverflow(void);

#endif /* __COMM_MANAGER_H */

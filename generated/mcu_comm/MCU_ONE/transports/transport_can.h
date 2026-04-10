/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  文 件 名: transport_can.h
  版 本 号: 1.0.0
  作    者: Xuan
  生成日期: 2026-03-09
  功能描述: Zephyr CAN 传输层头文件
  补    充: 协议版本 2.0.0

*******************************************************************************/

#ifndef TRANSPORT_CAN_H
#define TRANSPORT_CAN_H

#include "transport_interface.h"

int CAN_TransportInit(void);

extern Transport_interface_t g_can_transport;

#endif /* TRANSPORT_CAN_H */

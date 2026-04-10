/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  文 件 名: transport_uart.h
  版 本 号: 1.0.0
  作    者: Xuan
  生成日期: 2026-04-03
  功能描述: Zephyr UART 传输层头文件
  补    充: 协议版本 2.0.0

*******************************************************************************/

#ifndef TRANSPORT_UART_H
#define TRANSPORT_UART_H

#include "transport_interface.h"

int UART_TransportInit(void);
bool UART_RxBufferIsOverflowed(void);
void UART_RxBufferClearOverflowFlag(void);
bool UART_LinkIsActive(void);

extern Transport_interface_t g_uart_transport;

#endif /* TRANSPORT_UART_H */

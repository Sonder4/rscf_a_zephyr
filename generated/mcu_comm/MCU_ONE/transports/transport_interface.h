/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  文 件 名: transport_interface.h
  版 本 号: 1.0.0
  作    者: Xuan
  生成日期: 2026-02-11
  功能描述: Zephyr 传输层接口定义
  补    充: 协议版本 2.0.0

*******************************************************************************/

#ifndef TRANSPORT_INTERFACE_H
#define TRANSPORT_INTERFACE_H

#include <stdbool.h>
#include <stdint.h>

#define MCU_COMM_TRANSPORT_USB  0
#define MCU_COMM_TRANSPORT_CAN  1
#define MCU_COMM_TRANSPORT_UART 2

typedef enum
{
    MCU_COMM_UART_INSTANCE_NONE = 0,
    MCU_COMM_UART_INSTANCE_USART1,
    MCU_COMM_UART_INSTANCE_USART2,
    MCU_COMM_UART_INSTANCE_USART3,
    MCU_COMM_UART_INSTANCE_USART6,
    MCU_COMM_UART_INSTANCE_UART7,
    MCU_COMM_UART_INSTANCE_UART8,
} MCUCommUartInstance_e;

#ifndef MCU_COMM_DEFAULT_TRANSPORT
#define MCU_COMM_DEFAULT_TRANSPORT MCU_COMM_TRANSPORT_USB
#endif

#ifndef MCU_COMM_DEFAULT_UART_INSTANCE
#define MCU_COMM_DEFAULT_UART_INSTANCE MCU_COMM_UART_INSTANCE_UART7
#endif

#ifndef MCU_COMM_UART_BAUDRATE
#define MCU_COMM_UART_BAUDRATE 115200U
#endif

#ifndef MCU_COMM_UART_RX_BUFFER_SIZE
#define MCU_COMM_UART_RX_BUFFER_SIZE 256U
#endif

typedef struct
{
    int (*init)(void);
    int (*send)(uint8_t* data, uint16_t len);
    int (*receive)(uint8_t* buffer, uint16_t len);
    bool (*rx_overflowed)(void);
    void (*clear_rx_overflow)(void);
    bool (*is_connected)(void);
} Transport_interface_t;

#endif /* TRANSPORT_INTERFACE_H */

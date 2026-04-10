/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  文 件 名: comm_manager.c
  版 本 号: 1.0.0
  作    者: Xuan
  生成日期: 2026-02-11
  功能描述: Zephyr 通信管理模块源文件
  补    充: 协议版本 2.0.0

*******************************************************************************/

#include "comm_manager.h"

#include <stddef.h>
#include <string.h>

#include "bsp_log.h"
#include "transport_can.h"
#include "transport_uart.h"
#include "transport_usb.h"

#ifndef MCU_COMM_DEBUG_TRACE
#define MCU_COMM_DEBUG_TRACE 0
#endif

#if MCU_COMM_DEBUG_TRACE
#define COMM_TRACE_INFO(format, ...) LOGINFO("[comm] " format, ##__VA_ARGS__)
#define COMM_TRACE_WARN(format, ...) LOGWARNING("[comm] " format, ##__VA_ARGS__)
#else
#define COMM_TRACE_INFO(format, ...)
#define COMM_TRACE_WARN(format, ...)
#endif

static Transport_interface_t* g_transport = NULL;
static ProtocolFrame_t rx_frame;
static uint8_t tx_buffer[256];
static uint8_t is_batch_mode = 0;
static uint16_t batch_len = 0;
static uint32_t s_callback_count[256] = {0};
static uint32_t s_total_callback_count = 0;

void Comm_Init(Transport_interface_t* transport)
{
    g_transport = transport;
    if (g_transport == NULL)
    {
#if MCU_COMM_DEFAULT_TRANSPORT == MCU_COMM_TRANSPORT_CAN
        g_transport = &g_can_transport;
#elif MCU_COMM_DEFAULT_TRANSPORT == MCU_COMM_TRANSPORT_UART
        g_transport = &g_uart_transport;
#else
        g_transport = &g_usb_transport;
#endif
    }

    if ((g_transport != NULL) && (g_transport->init != NULL))
    {
        (void)g_transport->init();
    }

    ProtocolFrame_Init(&rx_frame);
    PID_Registry_Init();
}

bool Comm_IsConnected(void)
{
    if ((g_transport == NULL) || (g_transport->is_connected == NULL))
    {
        return false;
    }

    return g_transport->is_connected();
}

bool Comm_HasRxOverflow(void)
{
    if ((g_transport == NULL) || (g_transport->rx_overflowed == NULL))
    {
        return false;
    }

    return g_transport->rx_overflowed();
}

void Comm_Batch_Begin(void)
{
    is_batch_mode = 1U;
    batch_len = 0U;
}

void Comm_Send(uint8_t mid, uint8_t pid, const uint8_t* data, uint8_t len)
{
    int send_ret;

    if ((g_transport == NULL) || (g_transport->send == NULL))
    {
        COMM_TRACE_WARN("send skipped: transport not ready pid=0x%02X", pid);
        return;
    }

    if (is_batch_mode != 0U)
    {
        if (batch_len == 0U)
        {
            batch_len = Protocol_Pack_Begin(mid, tx_buffer);
        }

        if (((size_t)batch_len + 1U + 1U + (size_t)len + 3U) > sizeof(tx_buffer))
        {
            batch_len = Protocol_Pack_End(tx_buffer, batch_len);
            send_ret = g_transport->send(tx_buffer, batch_len);
            if (send_ret != 0)
            {
                COMM_TRACE_WARN("batch flush fail pid=0x%02X ret=%d len=%u", pid, send_ret, batch_len);
            }
            batch_len = Protocol_Pack_Begin(mid, tx_buffer);
        }

        batch_len = Protocol_Pack_Add(pid, data, len, tx_buffer, batch_len);
        return;
    }

    batch_len = Protocol_Pack(mid, pid, data, len, tx_buffer);
    send_ret = g_transport->send(tx_buffer, batch_len);
    if (send_ret != 0)
    {
        COMM_TRACE_WARN("tx fail pid=0x%02X ret=%d len=%u mid=0x%02X", pid, send_ret, batch_len, mid);
    }
}

void Comm_Batch_End(void)
{
    int send_ret;

    if ((is_batch_mode == 0U) || (batch_len == 0U))
    {
        is_batch_mode = 0U;
        batch_len = 0U;
        return;
    }

    batch_len = Protocol_Pack_End(tx_buffer, batch_len);
    send_ret = g_transport->send(tx_buffer, batch_len);
    if (send_ret != 0)
    {
        COMM_TRACE_WARN("batch end fail ret=%d len=%u", send_ret, batch_len);
    }
    is_batch_mode = 0U;
    batch_len = 0U;
}

void Comm_ProcessRx(void)
{
    uint8_t buf[64];
    int len;

    if ((g_transport == NULL) || (g_transport->receive == NULL))
    {
        return;
    }

    if (Comm_HasRxOverflow())
    {
        LOGERROR("[comm] transport RX overflow");
        if (g_transport->clear_rx_overflow != NULL)
        {
            g_transport->clear_rx_overflow();
        }
    }

    while ((len = g_transport->receive(buf, sizeof(buf))) > 0)
    {
        for (int i = 0; i < len; ++i)
        {
            int8_t status = Protocol_Unpack(buf[i], &rx_frame);
            if (status == 3)
            {
                uint8_t pid = rx_frame.pid;
                if (pid_registry[pid].callback != NULL)
                {
                    pid_registry[pid].callback(pid_registry[pid].data_ptr);
                    s_callback_count[pid]++;
                    s_total_callback_count++;
                }
            }
        }
    }
}

void Comm_Process(void)
{
    Comm_ProcessRx();
}

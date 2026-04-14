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

#include <errno.h>
#include <stddef.h>
#include <string.h>

#include <zephyr/sys/util.h>

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
static uint16_t s_compat_endpoint_map[256] = {0};
static uint8_t s_compat_endpoint_valid[256] = {0};

static Transport_interface_t* Comm_SelectDefaultTransport(void)
{
#if MCU_COMM_DEFAULT_TRANSPORT == MCU_COMM_TRANSPORT_CAN
    return &g_can_transport;
#elif MCU_COMM_DEFAULT_TRANSPORT == MCU_COMM_TRANSPORT_UART
    return &g_uart_transport;
#else
    return &g_usb_transport;
#endif
}

__attribute__((weak)) void Comm_OnValidRxFrame(uint8_t pid)
{
    (void)pid;
}

__attribute__((weak)) Transport_interface_t* Comm_ResolveTransportOrDefault(Transport_interface_t* transport)
{
    return transport;
}

__attribute__((weak)) void Comm_OnTransportResolved(Transport_interface_t* transport)
{
    ARG_UNUSED(transport);
}

__attribute__((weak)) uint16_t Comm_CompatMapEndpoint(uint8_t pid)
{
    if (s_compat_endpoint_valid[pid] != 0U)
    {
        return s_compat_endpoint_map[pid];
    }

    return (uint16_t)pid;
}

__attribute__((weak)) void Comm_RegisterCompatEndpoints(void)
{
}

__attribute__((weak)) void Comm_OnCompatEndpointResolved(uint8_t pid, uint16_t endpoint_id)
{
    ARG_UNUSED(pid);
    ARG_UNUSED(endpoint_id);
}

Transport_interface_t* Comm_GetTransport(void)
{
    return g_transport;
}

void Comm_RegisterCompatEndpoint(uint8_t pid, uint16_t endpoint_id)
{
    s_compat_endpoint_map[pid] = endpoint_id;
    s_compat_endpoint_valid[pid] = 1U;
}

int Comm_Init(Transport_interface_t* transport)
{
    int ret = 0;

    memset(s_compat_endpoint_map, 0, sizeof(s_compat_endpoint_map));
    memset(s_compat_endpoint_valid, 0, sizeof(s_compat_endpoint_valid));
    g_transport = transport;
    if (g_transport == NULL)
    {
        g_transport = Comm_SelectDefaultTransport();
    }

    g_transport = Comm_ResolveTransportOrDefault(g_transport);
    Comm_OnTransportResolved(g_transport);

    if ((g_transport != NULL) && (g_transport->init != NULL))
    {
        ret = g_transport->init();
    }

    ProtocolFrame_Init(&rx_frame);
    PID_Registry_Init();
    Comm_RegisterCompatEndpoints();
    return ret;
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
            if ((send_ret != 0) && (send_ret != -ENOTCONN))
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
    if ((send_ret != 0) && (send_ret != -ENOTCONN))
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
    if ((send_ret != 0) && (send_ret != -ENOTCONN))
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
                uint16_t compat_endpoint = Comm_CompatMapEndpoint(pid);
                Comm_OnCompatEndpointResolved(pid, compat_endpoint);
                Comm_OnValidRxFrame(pid);
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

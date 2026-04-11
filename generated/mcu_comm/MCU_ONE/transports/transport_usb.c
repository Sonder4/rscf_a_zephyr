/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  文 件 名: transport_usb.c
  版 本 号: 1.0.0
  作    者: Xuan
  生成日期: 2026-02-11
  功能描述: Zephyr USB 传输层实现
  补    充: 基于 CDC ACM 虚拟串口实现协议收发

*******************************************************************************/

#include "transport_usb.h"

#include <errno.h>
#include <stdbool.h>

#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/sys/util.h>

static const struct device* g_usb_uart_dev;
static bool g_usb_enabled;
static bool g_usb_ready;
static bool g_usb_overflowed;

bool USB_DeviceConfigured(void)
{
    return g_usb_ready;
}

bool USB_LinkIsActive(void)
{
    return g_usb_ready;
}

bool USB_RxBufferIsOverflowed(void)
{
    return g_usb_overflowed;
}

void USB_RxBufferClearOverflowFlag(void)
{
    g_usb_overflowed = false;
}

static bool USB_TransportIsConnected(void)
{
    return g_usb_ready;
}

int USB_Init(void)
{
    int ret;

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_cdc_acm_uart)
    g_usb_uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
    g_usb_ready = device_is_ready(g_usb_uart_dev);
#else
    g_usb_uart_dev = NULL;
    g_usb_ready = false;
#endif
    g_usb_overflowed = false;
    if (!g_usb_ready)
    {
        return -ENODEV;
    }

    if (!g_usb_enabled)
    {
        ret = usb_enable(NULL);
        if ((ret != 0) && (ret != -EALREADY))
        {
            g_usb_ready = false;
            return ret;
        }

        /* 旧 USB 栈在初始化早期写 DCD/DSR 可能触发 fault，这里只做枚举使能。 */
        g_usb_enabled = true;
    }

    return 0;
}

static int USB_Send(uint8_t* data, uint16_t len)
{
    if ((!g_usb_ready) || (data == NULL) || (len == 0U))
    {
        return -EINVAL;
    }

    if (!USB_TransportIsConnected())
    {
        return -ENOTCONN;
    }

    for (uint16_t i = 0U; i < len; ++i)
    {
        uart_poll_out(g_usb_uart_dev, data[i]);
    }

    return 0;
}

static int USB_Receive(uint8_t* buffer, uint16_t len)
{
    uint16_t count = 0U;
    unsigned char byte = 0U;

    if ((!g_usb_ready) || (buffer == NULL) || (len == 0U))
    {
        return 0;
    }

    while ((count < len) && (uart_poll_in(g_usb_uart_dev, &byte) == 0))
    {
        buffer[count++] = byte;
    }

    return (int)count;
}

Transport_interface_t g_usb_transport = {
    .init = USB_Init,
    .send = USB_Send,
    .receive = USB_Receive,
    .rx_overflowed = USB_RxBufferIsOverflowed,
    .clear_rx_overflow = USB_RxBufferClearOverflowFlag,
    .is_connected = USB_TransportIsConnected,
};

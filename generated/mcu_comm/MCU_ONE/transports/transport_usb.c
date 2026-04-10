/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

 *******************************************************************************
  文 件 名: transport_usb.c
  版 本 号: 1.0.0
  作    者: Xuan
  生成日期: 2026-02-11
  功能描述: Zephyr USB 传输层实现
  补    充: 当前阶段保留设备状态与占位接口

*******************************************************************************/

#include "transport_usb.h"

#include <errno.h>

#include <zephyr/sys/util.h>

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
    g_usb_ready = false;
    g_usb_overflowed = false;
    return -ENOTSUP;
}

static int USB_Send(uint8_t* data, uint16_t len)
{
    ARG_UNUSED(data);
    ARG_UNUSED(len);
    return -ENOTSUP;
}

static int USB_Receive(uint8_t* buffer, uint16_t len)
{
    ARG_UNUSED(buffer);
    ARG_UNUSED(len);
    return 0;
}

Transport_interface_t g_usb_transport = {
    .init = USB_Init,
    .send = USB_Send,
    .receive = USB_Receive,
    .rx_overflowed = USB_RxBufferIsOverflowed,
    .clear_rx_overflow = USB_RxBufferClearOverflowFlag,
    .is_connected = USB_TransportIsConnected,
};

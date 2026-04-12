#include <uxr/client/transport.h>

#if __has_include(<zephyr/version.h>)
#include <zephyr/version.h>
#else
#include <version.h>
#endif

#if ZEPHYR_VERSION_CODE >= ZEPHYR_VERSION(3,1,0)
#include <zephyr/kernel.h>
#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/sys/ring_buffer.h>
#include <zephyr/usb/usb_device.h>
#include <zephyr/posix/unistd.h>
#else 
#include <zephyr.h>
#include <device.h>
#include <sys/printk.h>
#include <drivers/uart.h>
#include <sys/ring_buffer.h>
#include <usb/usb_device.h>
#include <posix/unistd.h>
#endif

#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include <microros_transports.h>


#define RING_BUF_SIZE CONFIG_USB_CDC_ACM_RINGBUF_SIZE
#define USB_DTR_WAIT_STEP_MS 10
#define USB_DTR_WAIT_TIMEOUT_MS 1500

static uint8_t uart_in_buffer[RING_BUF_SIZE];
static bool g_usb_enabled;
static bool g_ring_ready;
static bool g_irq_ready;
zephyr_transport_params_t default_params;

struct ring_buf in_ringbuf;

static void uart_fifo_callback(const struct device *dev, void * user_data){
    while (uart_irq_update(dev) && uart_irq_is_pending(dev)) {
        if (uart_irq_rx_ready(dev)) {
            int recv_len;
            char buffer[64];
            size_t len = MIN(ring_buf_space_get(&in_ringbuf), sizeof(buffer));

            if (len > 0){
                recv_len = uart_fifo_read(dev, buffer, len);
                ring_buf_put(&in_ringbuf, buffer, recv_len);
            }

        }

        if (uart_irq_tx_ready(dev)) {
            uart_irq_tx_disable(dev);
        }
    }
}

bool zephyr_transport_open(struct uxrCustomTransport * transport){
    zephyr_transport_params_t * params = (zephyr_transport_params_t*) transport->args;
    int ret;
    uint32_t dtr = 0U;
    int wait_ms = 0;

#if DT_HAS_COMPAT_STATUS_OKAY(zephyr_cdc_acm_uart)
    params->uart_dev = DEVICE_DT_GET_ONE(zephyr_cdc_acm_uart);
#else
    params->uart_dev = NULL;
#endif

    if ((params->uart_dev == NULL) || !device_is_ready(params->uart_dev)) {
        return false;
    }

    if (!g_usb_enabled) {
        ret = usb_enable(NULL);
        if (ret != 0) {
            return false;
        }
        g_usb_enabled = true;
    }

    if (!g_ring_ready) {
        ring_buf_init(&in_ringbuf, sizeof(uart_in_buffer), uart_in_buffer);
        g_ring_ready = true;
    } else {
        ring_buf_reset(&in_ringbuf);
    }

    while (wait_ms < USB_DTR_WAIT_TIMEOUT_MS) {
        ret = uart_line_ctrl_get(params->uart_dev, UART_LINE_CTRL_DTR, &dtr);
        if ((ret == 0) && (dtr != 0U)) {
            break;
        }

        k_sleep(K_MSEC(USB_DTR_WAIT_STEP_MS));
        wait_ms += USB_DTR_WAIT_STEP_MS;
    }

    if (dtr == 0U) {
        return false;
    }

    if (!g_irq_ready) {
        uart_irq_callback_set(params->uart_dev, uart_fifo_callback);
        g_irq_ready = true;
    }

    uart_irq_tx_disable(params->uart_dev);
    uart_irq_rx_disable(params->uart_dev);
    uart_irq_rx_enable(params->uart_dev);

    return true;
}

bool zephyr_transport_close(struct uxrCustomTransport * transport){
    zephyr_transport_params_t * params = (zephyr_transport_params_t*) transport->args;
    if ((params != NULL) && (params->uart_dev != NULL) && g_irq_ready) {
        uart_irq_rx_disable(params->uart_dev);
        uart_irq_tx_disable(params->uart_dev);
    }

    return true;
}

size_t zephyr_transport_write(struct uxrCustomTransport* transport, const uint8_t * buf, size_t len, uint8_t * err){
    zephyr_transport_params_t * params = (zephyr_transport_params_t*) transport->args;

    if (err != NULL) {
        *err = 0U;
    }

    if ((params == NULL) || (params->uart_dev == NULL)) {
        if (err != NULL) {
            *err = 1U;
        }
        return 0U;
    }

    for (size_t i = 0U; i < len; ++i) {
        uart_poll_out(params->uart_dev, buf[i]);
    }

    return len;
}

size_t zephyr_transport_read(struct uxrCustomTransport* transport, uint8_t* buf, size_t len, int timeout, uint8_t* err){
    zephyr_transport_params_t * params = (zephyr_transport_params_t*) transport->args;
    size_t read = 0;
    int spent_time = 0;

    if (err != NULL) {
        *err = 0U;
    }

    if ((params == NULL) || (params->uart_dev == NULL)) {
        if (err != NULL) {
            *err = 1U;
        }
        return 0U;
    }

    while(ring_buf_is_empty(&in_ringbuf) && spent_time < timeout){
        k_sleep(K_MSEC(1));
        spent_time++;
    }

    uart_irq_rx_disable(params->uart_dev);
    read = ring_buf_get(&in_ringbuf, buf, len);
    uart_irq_rx_enable(params->uart_dev);

    return read;
}

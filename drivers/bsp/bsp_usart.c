#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "bsp_usart.h"

LOG_MODULE_REGISTER(bsp_usart, LOG_LEVEL_INF);

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;
UART_HandleTypeDef huart6;
UART_HandleTypeDef huart7;
UART_HandleTypeDef huart8;

static USARTInstance *s_usart_instances[DEVICE_USART_CNT];
static uint8_t s_usart_instance_count;
static bool s_worker_started;
static struct k_thread s_worker_thread;
static K_THREAD_STACK_DEFINE(s_worker_stack, 2048);

static const struct device *s_dev_usart1 = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(usart1));
static const struct device *s_dev_usart2 = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(usart2));
static const struct device *s_dev_usart3 = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(usart3));
static const struct device *s_dev_usart6 = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(usart6));
static const struct device *s_dev_uart7 = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(uart7));
static const struct device *s_dev_uart8 = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(uart8));

static void BSPUSARTFlushFrame(USARTInstance *instance)
{
    uint16_t frame_len;

    if ((instance == NULL) || (instance->rx_len == 0U))
    {
        return;
    }

    frame_len = instance->rx_len;
    instance->rx_len = 0U;
    if (instance->module_callback != NULL)
    {
        instance->module_callback(instance->module_callback_param, instance->recv_buff, frame_len);
    }
    memset(instance->recv_buff, 0, frame_len);
}

static const struct device *BSPUSARTResolveDevice(UART_HandleTypeDef *handle)
{
    if (handle == &huart1)
    {
        return s_dev_usart1;
    }
    if (handle == &huart2)
    {
        return s_dev_usart2;
    }
    if (handle == &huart3)
    {
        return s_dev_usart3;
    }
    if (handle == &huart6)
    {
        return s_dev_usart6;
    }
    if (handle == &huart7)
    {
        return s_dev_uart7;
    }
    if (handle == &huart8)
    {
        return s_dev_uart8;
    }

    return NULL;
}

static void BSPUSARTWorker(void *arg0, void *arg1, void *arg2)
{
    ARG_UNUSED(arg0);
    ARG_UNUSED(arg1);
    ARG_UNUSED(arg2);

    while (1)
    {
        uint32_t now = k_uptime_get_32();

        for (uint8_t index = 0U; index < s_usart_instance_count; ++index)
        {
            USARTInstance *instance = s_usart_instances[index];

            if ((instance == NULL) || (instance->uart_dev == NULL) || (instance->active == 0U))
            {
                continue;
            }

            while (instance->rx_len < instance->recv_buff_size)
            {
                unsigned char byte = 0U;

                if (uart_poll_in(instance->uart_dev, &byte) != 0)
                {
                    break;
                }

                instance->recv_buff[instance->rx_len++] = byte;
                instance->last_rx_tick = now;
            }

            if ((instance->rx_len >= instance->recv_buff_size) ||
                ((instance->rx_len > 0U) && ((uint32_t)(now - instance->last_rx_tick) >= 2U)))
            {
                BSPUSARTFlushFrame(instance);
            }
        }

        k_sleep(K_MSEC(1));
    }
}

static void BSPUSARTStartWorker(void)
{
    if (s_worker_started)
    {
        return;
    }

    k_thread_create(
        &s_worker_thread,
        s_worker_stack,
        K_THREAD_STACK_SIZEOF(s_worker_stack),
        BSPUSARTWorker,
        NULL,
        NULL,
        NULL,
        K_PRIO_PREEMPT(12),
        0,
        K_NO_WAIT
    );
    s_worker_started = true;
}

void USARTServiceInit(USARTInstance *_instance)
{
    if (_instance == NULL)
    {
        return;
    }

    _instance->rx_len = 0U;
    _instance->last_rx_tick = k_uptime_get_32();
    _instance->active = 1U;
    memset(_instance->recv_buff, 0, sizeof(_instance->recv_buff));
}

USARTInstance *USARTRegister(USART_Init_Config_s *init_config)
{
    USARTInstance *instance;

    if ((init_config == NULL) || (init_config->usart_handle == NULL) || (init_config->recv_buff_size == 0U))
    {
        return NULL;
    }

    if ((s_usart_instance_count >= DEVICE_USART_CNT) || (init_config->recv_buff_size > USART_RXBUFF_LIMIT))
    {
        LOG_ERR("usart register failed: count=%u size=%u",
                (unsigned int)s_usart_instance_count,
                (unsigned int)init_config->recv_buff_size);
        return NULL;
    }

    instance = (USARTInstance *)malloc(sizeof(USARTInstance));
    if (instance == NULL)
    {
        return NULL;
    }

    memset(instance, 0, sizeof(USARTInstance));
    instance->usart_handle = init_config->usart_handle;
    instance->recv_buff_size = init_config->recv_buff_size;
    instance->module_callback = init_config->module_callback;
    instance->module_callback_param = init_config->moduleinstance;
    instance->uart_dev = BSPUSARTResolveDevice(init_config->usart_handle);
    if ((instance->uart_dev == NULL) || (!device_is_ready(instance->uart_dev)))
    {
        LOG_WRN("uart backend is not ready");
        free(instance);
        return NULL;
    }

    k_mutex_init(&instance->tx_lock);
    s_usart_instances[s_usart_instance_count++] = instance;
    USARTServiceInit(instance);
    BSPUSARTStartWorker();
    return instance;
}

void USARTSend(USARTInstance *_instance, uint8_t *send_buf, uint16_t send_size, USART_TRANSFER_MODE mode)
{
    if ((_instance == NULL) || (_instance->uart_dev == NULL) || (send_buf == NULL) || (send_size == 0U))
    {
        return;
    }

    ARG_UNUSED(mode);
    k_mutex_lock(&_instance->tx_lock, K_FOREVER);
    for (uint16_t index = 0U; index < send_size; ++index)
    {
        uart_poll_out(_instance->uart_dev, send_buf[index]);
    }
    k_mutex_unlock(&_instance->tx_lock);
}

uint8_t USARTIsReady(USARTInstance *_instance)
{
    if ((_instance == NULL) || (_instance->uart_dev == NULL))
    {
        return 0U;
    }

    return 1U;
}

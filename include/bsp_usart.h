#ifndef BSP_USART_H_
#define BSP_USART_H_

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "main.h"

#define DEVICE_USART_CNT 8U
#define USART_RXBUFF_LIMIT 256U

typedef void (*usart_module_callback)(void *instance, uint8_t *data, uint16_t len);

typedef enum
{
    USART_TRANSFER_NONE = 0,
    USART_TRANSFER_BLOCKING,
    USART_TRANSFER_IT,
    USART_TRANSFER_DMA,
} USART_TRANSFER_MODE;

typedef struct usart_instance
{
    uint8_t recv_buff[USART_RXBUFF_LIMIT];
    uint8_t recv_buff_size;
    UART_HandleTypeDef *usart_handle;
    usart_module_callback module_callback;
    void *module_callback_param;
    const struct device *uart_dev;
    uint16_t rx_len;
    uint32_t last_rx_tick;
    struct k_mutex tx_lock;
    uint8_t active;
} USARTInstance;

typedef struct
{
    uint8_t recv_buff_size;
    UART_HandleTypeDef *usart_handle;
    usart_module_callback module_callback;
    void *moduleinstance;
} USART_Init_Config_s;

USARTInstance *USARTRegister(USART_Init_Config_s *init_config);
void USARTServiceInit(USARTInstance *_instance);
void USARTSend(USARTInstance *_instance, uint8_t *send_buf, uint16_t send_size, USART_TRANSFER_MODE mode);
uint8_t USARTIsReady(USARTInstance *_instance);

#endif /* BSP_USART_H_ */

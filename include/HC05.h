#ifndef HC05_H_
#define HC05_H_

#include <stdint.h>

#include "bsp_usart.h"

#define HC05_DATASIZE 4U

typedef struct
{
    uint8_t send_data[HC05_DATASIZE + 2U];
    uint8_t recv_data[HC05_DATASIZE];
    uint32_t update_tick;
    uint32_t sample_seq;
    uint8_t online;
} HC05;

HC05 *HC05Init(UART_HandleTypeDef *hc05_usart_handle);
void HC05_SendData(uint8_t *data, uint8_t data_num);
uint8_t HC05TryGetLatest(HC05 *out);
uint8_t HC05IsOnline(void);

#endif /* HC05_H_ */

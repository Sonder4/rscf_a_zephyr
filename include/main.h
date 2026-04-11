#ifndef MAIN_H_
#define MAIN_H_

#include <stdint.h>

#include <zephyr/sys/util.h>

#include <stm32f4xx_hal_can.h>
#include <stm32f4xx_hal_uart.h>

extern CAN_HandleTypeDef hcan1;
extern CAN_HandleTypeDef hcan2;

extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart6;
extern UART_HandleTypeDef huart7;
extern UART_HandleTypeDef huart8;

#ifndef UNUSED
#define UNUSED(x) ARG_UNUSED(x)
#endif

static inline void Error_Handler(void)
{
}

#endif /* MAIN_H_ */

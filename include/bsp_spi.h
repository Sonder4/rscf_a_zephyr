#ifndef BSP_SPI_H_
#define BSP_SPI_H_

#include <stdint.h>

#include "gpio.h"
#include "spi.h"

#define SPI_DEVICE_CNT 4U
#define MX_SPI_BUS_SLAVE_CNT 4U

typedef enum
{
    SPI_BLOCK_MODE = 0,
    SPI_IT_MODE,
    SPI_DMA_MODE,
} SPI_TXRX_MODE_e;

struct spi_instance;

typedef void (*spi_rx_callback)(struct spi_instance *instance);

typedef struct spi_instance
{
    SPI_HandleTypeDef *spi_handle;
    GPIO_TypeDef *GPIOx;
    uint16_t cs_pin;
    SPI_TXRX_MODE_e spi_work_mode;
    uint8_t rx_size;
    uint8_t *rx_buffer;
    uint8_t CS_State;
    uint8_t *cs_pin_state;
    spi_rx_callback callback;
    void *id;
} SPIInstance;

typedef struct
{
    SPI_HandleTypeDef *spi_handle;
    GPIO_TypeDef *GPIOx;
    uint16_t cs_pin;
    SPI_TXRX_MODE_e spi_work_mode;
    spi_rx_callback callback;
    void *id;
} SPI_Init_Config_s;

SPIInstance *SPIRegister(SPI_Init_Config_s *conf);
void SPITransmit(SPIInstance *spi_ins, uint8_t *ptr_data, uint8_t len);
void SPIRecv(SPIInstance *spi_ins, uint8_t *ptr_data, uint8_t len);
void SPITransRecv(SPIInstance *spi_ins, uint8_t *ptr_data_rx, uint8_t *ptr_data_tx, uint8_t len);
void SPISetMode(SPIInstance *spi_ins, SPI_TXRX_MODE_e spi_mode);

#endif /* BSP_SPI_H_ */

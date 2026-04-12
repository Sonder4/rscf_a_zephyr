#include <errno.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "bsp_spi.h"

LOG_MODULE_REGISTER(bsp_spi, LOG_LEVEL_INF);

SPI_HandleTypeDef hspi4;
SPI_HandleTypeDef hspi5;

static SPIInstance *s_spi_instances[SPI_DEVICE_CNT];
static uint8_t s_spi_instance_count;
static uint8_t s_spi_bus_idle[SPI_DEVICE_CNT] = {1U, 1U, 1U, 1U};
static struct k_mutex s_spi_bus_lock[SPI_DEVICE_CNT];
static bool s_spi_lock_ready;

static const struct device *s_spi4_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(spi4));
static const struct device *s_spi5_dev = DEVICE_DT_GET_OR_NULL(DT_NODELABEL(spi5));

static void BSPSPIEnsureInit(void)
{
    if (s_spi_lock_ready)
    {
        return;
    }

    memset(&hspi4, 0, sizeof(hspi4));
    memset(&hspi5, 0, sizeof(hspi5));
    hspi4.Instance = SPI4;
    hspi5.Instance = SPI5;

    for (size_t index = 0U; index < ARRAY_SIZE(s_spi_bus_lock); ++index)
    {
        k_mutex_init(&s_spi_bus_lock[index]);
    }

    s_spi_lock_ready = true;
}

static int BSPSPIResolveBusIndex(SPI_HandleTypeDef *handle)
{
    if (handle == &hspi4)
    {
        return 0;
    }
    if (handle == &hspi5)
    {
        return 1;
    }

    return -1;
}

static const struct device *BSPSPIResolveDevice(SPI_HandleTypeDef *handle)
{
    if (handle == &hspi4)
    {
        return s_spi4_dev;
    }
    if (handle == &hspi5)
    {
        return s_spi5_dev;
    }

    return NULL;
}

static const struct device *BSPSPIResolveGpioDevice(GPIO_TypeDef *port)
{
    if (port == GPIOA)
    {
        return DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioa));
    }
    if (port == GPIOB)
    {
        return DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiob));
    }
    if (port == GPIOC)
    {
        return DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioc));
    }
    if (port == GPIOD)
    {
        return DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiod));
    }
    if (port == GPIOE)
    {
        return DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioe));
    }
    if (port == GPIOF)
    {
        return DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiof));
    }
    if (port == GPIOG)
    {
        return DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpiog));
    }
    if (port == GPIOH)
    {
        return DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioh));
    }
    if (port == GPIOI)
    {
        return DEVICE_DT_GET_OR_NULL(DT_NODELABEL(gpioi));
    }

    return NULL;
}

static gpio_pin_t BSPSPIResolvePinIndex(uint16_t pin_mask)
{
    for (gpio_pin_t pin = 0U; pin < 16U; ++pin)
    {
        if (pin_mask == BIT(pin))
        {
            return pin;
        }
    }

    return UINT8_MAX;
}

static void BSPSPISetCs(GPIO_TypeDef *port, uint16_t pin_mask, int level)
{
    const struct device *gpio_dev;
    gpio_pin_t pin;

    gpio_dev = BSPSPIResolveGpioDevice(port);
    pin = BSPSPIResolvePinIndex(pin_mask);
    if ((gpio_dev == NULL) || !device_is_ready(gpio_dev) || (pin == UINT8_MAX))
    {
        return;
    }

    (void)gpio_pin_set(gpio_dev, pin, level);
}

static int BSPSPITransfer(
    SPIInstance *spi_ins,
    uint8_t *ptr_data_rx,
    const uint8_t *ptr_data_tx,
    uint8_t len,
    bool rx_only,
    bool tx_only
)
{
    const struct device *spi_dev;
    struct spi_config spi_cfg;
    struct spi_buf tx_buf;
    struct spi_buf rx_buf;
    struct spi_buf_set tx_set;
    struct spi_buf_set rx_set;
    int bus_index;
    int ret;

    if ((spi_ins == NULL) || (spi_ins->spi_handle == NULL) || (len == 0U))
    {
        return -EINVAL;
    }

    spi_dev = BSPSPIResolveDevice(spi_ins->spi_handle);
    if ((spi_dev == NULL) || !device_is_ready(spi_dev))
    {
        return -ENODEV;
    }

    bus_index = BSPSPIResolveBusIndex(spi_ins->spi_handle);
    if ((bus_index < 0) || ((size_t)bus_index >= ARRAY_SIZE(s_spi_bus_lock)))
    {
        return -EINVAL;
    }

    memset(&spi_cfg, 0, sizeof(spi_cfg));
    spi_cfg.operation = SPI_OP_MODE_MASTER | SPI_WORD_SET(8) | SPI_TRANSFER_MSB;
    if (spi_ins->spi_handle == &hspi4)
    {
        spi_cfg.frequency = 42000000U;
    }
    else
    {
        spi_cfg.frequency = 328125U;
        spi_cfg.operation |= SPI_MODE_CPOL | SPI_MODE_CPHA;
    }

    k_mutex_lock(&s_spi_bus_lock[bus_index], K_FOREVER);
    s_spi_bus_idle[bus_index] = 0U;
    if (spi_ins->cs_pin_state != NULL)
    {
        *spi_ins->cs_pin_state = 0U;
    }

    BSPSPISetCs(spi_ins->GPIOx, spi_ins->cs_pin, 0);

    memset(&tx_buf, 0, sizeof(tx_buf));
    memset(&rx_buf, 0, sizeof(rx_buf));
    memset(&tx_set, 0, sizeof(tx_set));
    memset(&rx_set, 0, sizeof(rx_set));

    if (!rx_only)
    {
        tx_buf.buf = (void *)ptr_data_tx;
        tx_buf.len = len;
        tx_set.buffers = &tx_buf;
        tx_set.count = 1U;
    }

    if (!tx_only)
    {
        rx_buf.buf = ptr_data_rx;
        rx_buf.len = len;
        rx_set.buffers = &rx_buf;
        rx_set.count = 1U;
    }

    if (tx_only)
    {
        ret = spi_write(spi_dev, &spi_cfg, &tx_set);
    }
    else if (rx_only)
    {
        ret = spi_read(spi_dev, &spi_cfg, &rx_set);
    }
    else
    {
        ret = spi_transceive(spi_dev, &spi_cfg, &tx_set, &rx_set);
    }

    BSPSPISetCs(spi_ins->GPIOx, spi_ins->cs_pin, 1);
    s_spi_bus_idle[bus_index] = 1U;
    if (spi_ins->cs_pin_state != NULL)
    {
        *spi_ins->cs_pin_state = 1U;
    }
    k_mutex_unlock(&s_spi_bus_lock[bus_index]);

    if ((ret == 0) && (spi_ins->spi_work_mode != SPI_BLOCK_MODE) && (spi_ins->callback != NULL))
    {
        spi_ins->callback(spi_ins);
    }

    return ret;
}

SPIInstance *SPIRegister(SPI_Init_Config_s *conf)
{
    SPIInstance *instance;
    const struct device *gpio_dev;
    gpio_pin_t gpio_pin;
    int bus_index;

    BSPSPIEnsureInit();

    if ((conf == NULL) || (conf->spi_handle == NULL))
    {
        return NULL;
    }

    if (s_spi_instance_count >= ARRAY_SIZE(s_spi_instances))
    {
        LOG_ERR("spi instance count exceeded");
        return NULL;
    }

    bus_index = BSPSPIResolveBusIndex(conf->spi_handle);
    if ((bus_index < 0) || (bus_index >= (int)ARRAY_SIZE(s_spi_bus_idle)))
    {
        LOG_ERR("unsupported spi handle");
        return NULL;
    }

    instance = (SPIInstance *)malloc(sizeof(SPIInstance));
    if (instance == NULL)
    {
        return NULL;
    }

    memset(instance, 0, sizeof(*instance));
    instance->spi_handle = conf->spi_handle;
    instance->GPIOx = conf->GPIOx;
    instance->cs_pin = conf->cs_pin;
    instance->spi_work_mode = conf->spi_work_mode;
    instance->callback = conf->callback;
    instance->id = conf->id;
    instance->cs_pin_state = &s_spi_bus_idle[bus_index];

    gpio_dev = BSPSPIResolveGpioDevice(conf->GPIOx);
    gpio_pin = BSPSPIResolvePinIndex(conf->cs_pin);
    if ((gpio_dev != NULL) && device_is_ready(gpio_dev) && (gpio_pin != UINT8_MAX))
    {
        (void)gpio_pin_configure(gpio_dev, gpio_pin, GPIO_OUTPUT_HIGH);
    }

    s_spi_instances[s_spi_instance_count++] = instance;
    return instance;
}

void SPITransmit(SPIInstance *spi_ins, uint8_t *ptr_data, uint8_t len)
{
    (void)BSPSPITransfer(spi_ins, NULL, ptr_data, len, false, true);
}

void SPIRecv(SPIInstance *spi_ins, uint8_t *ptr_data, uint8_t len)
{
    if (spi_ins != NULL)
    {
        spi_ins->rx_size = len;
        spi_ins->rx_buffer = ptr_data;
    }
    (void)BSPSPITransfer(spi_ins, ptr_data, NULL, len, true, false);
}

void SPITransRecv(SPIInstance *spi_ins, uint8_t *ptr_data_rx, uint8_t *ptr_data_tx, uint8_t len)
{
    if (spi_ins != NULL)
    {
        spi_ins->rx_size = len;
        spi_ins->rx_buffer = ptr_data_rx;
    }
    (void)BSPSPITransfer(spi_ins, ptr_data_rx, ptr_data_tx, len, false, false);
}

void SPISetMode(SPIInstance *spi_ins, SPI_TXRX_MODE_e spi_mode)
{
    if (spi_ins == NULL)
    {
        return;
    }

    spi_ins->spi_work_mode = spi_mode;
}

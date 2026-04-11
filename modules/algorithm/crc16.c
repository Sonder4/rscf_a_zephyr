#include "crc16.h"

#include <stddef.h>

static uint8_t s_crc16_inited;
static uint16_t s_crc16_table[256];

void init_crc16_tab(void)
{
    uint16_t crc;

    for (uint16_t index = 0U; index < 256U; ++index)
    {
        crc = index;
        for (uint8_t bit = 0U; bit < 8U; ++bit)
        {
            if ((crc & 0x0001U) != 0U)
            {
                crc = (crc >> 1U) ^ CRC_POLY_16;
            }
            else
            {
                crc >>= 1U;
            }
        }
        s_crc16_table[index] = crc;
    }

    s_crc16_inited = 1U;
}

static uint16_t crc16_common(uint16_t seed, const uint8_t *input_str, uint16_t num_bytes)
{
    uint16_t crc = seed;

    if (!s_crc16_inited)
    {
        init_crc16_tab();
    }

    if (input_str == NULL)
    {
        return crc;
    }

    for (uint16_t index = 0U; index < num_bytes; ++index)
    {
        crc = (crc >> 8U) ^ s_crc16_table[(crc ^ (uint16_t)input_str[index]) & 0x00FFU];
    }

    return crc;
}

uint16_t crc_16(const uint8_t *input_str, uint16_t num_bytes)
{
    return crc16_common(CRC_START_16, input_str, num_bytes);
}

uint16_t crc_modbus(const uint8_t *input_str, uint16_t num_bytes)
{
    return crc16_common(CRC_START_MODBUS, input_str, num_bytes);
}

uint16_t update_crc_16(uint16_t crc, uint8_t c)
{
    if (!s_crc16_inited)
    {
        init_crc16_tab();
    }

    return (crc >> 8U) ^ s_crc16_table[(crc ^ (uint16_t)c) & 0x00FFU];
}

uint16_t Calculate_CRC16(const uint8_t *pData, uint32_t Size)
{
    return crc_16(pData, (uint16_t)Size);
}

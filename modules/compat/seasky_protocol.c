#include "seasky_protocol.h"

#include <string.h>

#include "crc16.h"
#include "crc8.h"

static uint8_t CRC8_Check_Sum(uint8_t *pchMessage, uint16_t dwLength)
{
    uint8_t expected;

    if ((pchMessage == NULL) || (dwLength <= 2U))
    {
        return 0U;
    }

    expected = crc_8(pchMessage, (uint16_t)(dwLength - 1U));
    return (expected == pchMessage[dwLength - 1U]) ? 1U : 0U;
}

static uint16_t CRC16_Check_Sum(uint8_t *pchMessage, uint32_t dwLength)
{
    uint16_t expected;

    if ((pchMessage == NULL) || (dwLength <= 2U))
    {
        return 0U;
    }

    expected = crc_16(pchMessage, (uint16_t)(dwLength - 2U));
    return (((expected & 0xFFU) == pchMessage[dwLength - 2U]) &&
            (((expected >> 8) & 0xFFU) == pchMessage[dwLength - 1U])) ? 1U : 0U;
}

static uint8_t protocol_heade_Check(protocol_rm_struct *pro, uint8_t *rx_buf)
{
    if ((pro == NULL) || (rx_buf == NULL))
    {
        return 0U;
    }

    if ((rx_buf[0] == PROTOCOL_CMD_ID) && CRC8_Check_Sum(&rx_buf[0], 4U))
    {
        pro->header.sof = rx_buf[0];
        pro->header.data_length = (uint16_t)(((uint16_t)rx_buf[2] << 8) | rx_buf[1]);
        pro->header.crc_check = rx_buf[3];
        pro->cmd_id = (uint16_t)(((uint16_t)rx_buf[5] << 8) | rx_buf[4]);
        return 1U;
    }

    return 0U;
}

void get_protocol_send_data(
    uint16_t send_id,
    uint16_t flags_register,
    float *tx_data,
    uint8_t float_length,
    uint8_t *tx_buf,
    uint16_t *tx_buf_len
)
{
    uint16_t crc16;
    uint16_t data_len;

    if ((tx_data == NULL) || (tx_buf == NULL) || (tx_buf_len == NULL))
    {
        return;
    }

    data_len = (uint16_t)(float_length * 4U + 2U);
    tx_buf[0] = PROTOCOL_CMD_ID;
    tx_buf[1] = (uint8_t)(data_len & 0xFFU);
    tx_buf[2] = (uint8_t)((data_len >> 8) & 0xFFU);
    tx_buf[3] = crc_8(&tx_buf[0], 3U);
    tx_buf[4] = (uint8_t)(send_id & 0xFFU);
    tx_buf[5] = (uint8_t)((send_id >> 8) & 0xFFU);
    tx_buf[6] = (uint8_t)(flags_register & 0xFFU);
    tx_buf[7] = (uint8_t)((flags_register >> 8) & 0xFFU);

    for (uint8_t index = 0U; index < (uint8_t)(4U * float_length); ++index)
    {
        tx_buf[index + 8U] = ((uint8_t *)(&tx_data[index / 4U]))[index % 4U];
    }

    crc16 = crc_16(&tx_buf[0], (uint16_t)(data_len + 6U));
    tx_buf[data_len + 6U] = (uint8_t)(crc16 & 0xFFU);
    tx_buf[data_len + 7U] = (uint8_t)((crc16 >> 8) & 0xFFU);
    *tx_buf_len = (uint16_t)(data_len + 8U);
}

uint16_t get_protocol_info(uint8_t *rx_buf, uint16_t *flags_register, uint8_t *rx_data)
{
    static protocol_rm_struct pro;
    uint16_t data_length;

    if ((rx_buf == NULL) || (flags_register == NULL) || (rx_data == NULL))
    {
        return 0U;
    }

    if (protocol_heade_Check(&pro, rx_buf) == 0U)
    {
        return 0U;
    }

    data_length = (uint16_t)(OFFSET_BYTE + pro.header.data_length);
    if (CRC16_Check_Sum(&rx_buf[0], data_length) == 0U)
    {
        return 0U;
    }

    *flags_register = (uint16_t)(((uint16_t)rx_buf[7] << 8) | rx_buf[6]);
    memcpy(rx_data, rx_buf + 8, (size_t)(pro.header.data_length - 2U));
    return pro.cmd_id;
}

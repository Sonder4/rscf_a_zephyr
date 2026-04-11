#ifndef SEASKY_PROTOCOL_H_
#define SEASKY_PROTOCOL_H_

#include <stdint.h>

#define PROTOCOL_CMD_ID 0xA5U
#define OFFSET_BYTE 8U

typedef struct
{
    struct
    {
        uint8_t sof;
        uint16_t data_length;
        uint8_t crc_check;
    } header;
    uint16_t cmd_id;
    uint16_t frame_tail;
} protocol_rm_struct;

void get_protocol_send_data(
    uint16_t send_id,
    uint16_t flags_register,
    float *tx_data,
    uint8_t float_length,
    uint8_t *tx_buf,
    uint16_t *tx_buf_len
);
uint16_t get_protocol_info(uint8_t *rx_buf, uint16_t *flags_register, uint8_t *rx_data);

#endif /* SEASKY_PROTOCOL_H_ */

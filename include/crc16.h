#ifndef CRC16_H_
#define CRC16_H_

#include <stdint.h>

#define CRC_START_16 0xFFFFU
#define CRC_START_MODBUS 0xFFFFU
#define CRC_POLY_16 0xA001U

uint16_t crc_16(const uint8_t *input_str, uint16_t num_bytes);
uint16_t crc_modbus(const uint8_t *input_str, uint16_t num_bytes);
uint16_t update_crc_16(uint16_t crc, uint8_t c);
void init_crc16_tab(void);
uint16_t Calculate_CRC16(const uint8_t *pData, uint32_t Size);

#endif /* CRC16_H_ */

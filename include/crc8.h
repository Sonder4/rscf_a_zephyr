#ifndef CRC8_H_
#define CRC8_H_

#include <stdint.h>

#define CRC_START_8 0x00U

uint8_t crc_8(const uint8_t *input_str, uint16_t num_bytes);
uint8_t update_crc_8(uint8_t crc, uint8_t val);

#endif /* CRC8_H_ */

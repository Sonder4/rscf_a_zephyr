#ifndef AD7190_H_
#define AD7190_H_

#include <stdint.h>

#define AD7190_CHANNEL_COUNT 4U

typedef struct
{
    float voltage_mv[AD7190_CHANNEL_COUNT];
    uint32_t update_tick;
    uint32_t sample_seq;
    uint8_t ready;
} AD7190_Frame_s;

int AD7190Init(void);
void AD7190Task(void);
uint8_t AD7190TryGetLatest(AD7190_Frame_s *out);
void AD7190SetMockVoltage(uint8_t channel, float voltage_mv);

#endif /* AD7190_H_ */

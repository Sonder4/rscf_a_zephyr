#ifndef OLED_H_
#define OLED_H_

#include <stdint.h>

#define OLED_LINE_COUNT 4U
#define OLED_LINE_LEN 16U

int OLEDInit(void);
void OLEDClear(void);
void OLEDShowString(uint8_t row, uint8_t col, const char *str);
const char *OLEDGetLine(uint8_t row);
void OLEDTask(void);

#endif /* OLED_H_ */

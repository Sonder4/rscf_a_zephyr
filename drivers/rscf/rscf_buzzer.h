#ifndef RSCF_BUZZER_H_
#define RSCF_BUZZER_H_

#include <stdint.h>

int RSCFBuzzerInit(void);
int RSCFBuzzerSetTone(uint32_t frequency_hz, uint16_t duty_permille);
void RSCFBuzzerStop(void);

#endif /* RSCF_BUZZER_H_ */

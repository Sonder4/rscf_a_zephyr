#ifndef RSCF_DTOF2510_H_
#define RSCF_DTOF2510_H_

#include <stdbool.h>
#include <stdint.h>

#define RSCF_DTOF2510_MAX_CHANNELS 4U
#define RSCF_DTOF2510_HISTORY_DEPTH 5U

struct rscf_dtof2510_frame {
  float distance_raw_mm_f;
  uint16_t distance_mm;
  uint16_t confidence;
  uint8_t valid;
};

int RSCFDtof2510Init(void);
int RSCFDtof2510InjectFrame(uint8_t channel, const char *line);
const struct rscf_dtof2510_frame *RSCFDtof2510Latest(uint8_t channel);
bool RSCFDtof2510Ready(void);

#endif /* RSCF_DTOF2510_H_ */

#ifndef RSCF_POWER_SWITCH_H_
#define RSCF_POWER_SWITCH_H_

#include <stdbool.h>
#include <stdint.h>

enum rscf_power_switch_channel {
  RSCF_POWER_SWITCH_1 = 0,
  RSCF_POWER_SWITCH_2,
  RSCF_POWER_SWITCH_3,
  RSCF_POWER_SWITCH_4,
  RSCF_POWER_SWITCH_CHANNEL_COUNT,
};

int RSCFPowerSwitchInit(void);
int RSCFPowerSwitchSet(enum rscf_power_switch_channel channel, bool enable);
int RSCFPowerSwitchSetMask(uint8_t enable_mask);
bool RSCFPowerSwitchGet(enum rscf_power_switch_channel channel);
uint8_t RSCFPowerSwitchGetMask(void);

#endif /* RSCF_POWER_SWITCH_H_ */

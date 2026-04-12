#include "rscf_power_switch.h"

#include <errno.h>

#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(rscf_power_switch, LOG_LEVEL_INF);

#define RSCF_POWER_SWITCH_NODE DT_NODELABEL(rscf_power_switches)

#if DT_NODE_HAS_STATUS(RSCF_POWER_SWITCH_NODE, okay)
static const struct gpio_dt_spec s_power_switches[RSCF_POWER_SWITCH_CHANNEL_COUNT] = {
  [RSCF_POWER_SWITCH_1] = GPIO_DT_SPEC_GET_BY_IDX(RSCF_POWER_SWITCH_NODE, power_gpios, 0),
  [RSCF_POWER_SWITCH_2] = GPIO_DT_SPEC_GET_BY_IDX(RSCF_POWER_SWITCH_NODE, power_gpios, 1),
  [RSCF_POWER_SWITCH_3] = GPIO_DT_SPEC_GET_BY_IDX(RSCF_POWER_SWITCH_NODE, power_gpios, 2),
  [RSCF_POWER_SWITCH_4] = GPIO_DT_SPEC_GET_BY_IDX(RSCF_POWER_SWITCH_NODE, power_gpios, 3),
};

static bool s_ready[RSCF_POWER_SWITCH_CHANNEL_COUNT];
static bool s_inited;
static uint8_t s_enable_mask;
#endif

static bool RSCFPowerSwitchChannelValid(enum rscf_power_switch_channel channel)
{
  return (channel >= RSCF_POWER_SWITCH_1) && (channel < RSCF_POWER_SWITCH_CHANNEL_COUNT);
}

int RSCFPowerSwitchSet(enum rscf_power_switch_channel channel, bool enable)
{
#if DT_NODE_HAS_STATUS(RSCF_POWER_SWITCH_NODE, okay)
  int ret;

  if (!RSCFPowerSwitchChannelValid(channel)) {
    return -EINVAL;
  }

  if (!s_inited || !s_ready[channel]) {
    return -ENODEV;
  }

  ret = gpio_pin_set_dt(&s_power_switches[channel], enable ? 1 : 0);
  if (ret != 0) {
    return ret;
  }

  WRITE_BIT(s_enable_mask, channel, enable ? 1U : 0U);
  return 0;
#else
  ARG_UNUSED(channel);
  ARG_UNUSED(enable);
  return 0;
#endif
}

int RSCFPowerSwitchSetMask(uint8_t enable_mask)
{
  int ret;

  for (size_t i = 0; i < RSCF_POWER_SWITCH_CHANNEL_COUNT; ++i) {
    ret = RSCFPowerSwitchSet((enum rscf_power_switch_channel)i,
                             ((enable_mask >> i) & 0x01U) != 0U);
    if (ret != 0) {
      return ret;
    }
  }

  return 0;
}

bool RSCFPowerSwitchGet(enum rscf_power_switch_channel channel)
{
#if DT_NODE_HAS_STATUS(RSCF_POWER_SWITCH_NODE, okay)
  if (!RSCFPowerSwitchChannelValid(channel)) {
    return false;
  }

  return ((s_enable_mask >> channel) & 0x01U) != 0U;
#else
  ARG_UNUSED(channel);
  return false;
#endif
}

uint8_t RSCFPowerSwitchGetMask(void)
{
#if DT_NODE_HAS_STATUS(RSCF_POWER_SWITCH_NODE, okay)
  return s_enable_mask;
#else
  return 0U;
#endif
}

int RSCFPowerSwitchInit(void)
{
#if DT_NODE_HAS_STATUS(RSCF_POWER_SWITCH_NODE, okay)
  int ret;

  s_enable_mask = 0U;
  for (size_t i = 0; i < ARRAY_SIZE(s_power_switches); ++i) {
    if (!device_is_ready(s_power_switches[i].port)) {
      LOG_WRN("power switch %u is not ready", (unsigned int)i);
      s_ready[i] = false;
      continue;
    }

    ret = gpio_pin_configure_dt(&s_power_switches[i], GPIO_OUTPUT_INACTIVE);
    if (ret != 0) {
      return ret;
    }

    s_ready[i] = true;
  }

  s_inited = true;
  ret = RSCFPowerSwitchSetMask((uint8_t)DT_PROP_OR(RSCF_POWER_SWITCH_NODE, default_state_mask, 0));
  if (ret != 0) {
    return ret;
  }

  LOG_INF("power switches initialized");
  return 0;
#else
  LOG_INF("power switch node disabled");
  return 0;
#endif
}

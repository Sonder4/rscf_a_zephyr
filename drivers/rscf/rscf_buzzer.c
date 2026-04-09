#include "rscf_buzzer.h"

#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(rscf_buzzer, LOG_LEVEL_INF);

#define RSCF_BUZZER_NODE DT_NODELABEL(rscf_buzzer)

#if DT_NODE_HAS_STATUS(RSCF_BUZZER_NODE, okay)
static const struct pwm_dt_spec s_buzzer = PWM_DT_SPEC_GET(RSCF_BUZZER_NODE);
static const uint32_t s_startup_freq_hz = DT_PROP(RSCF_BUZZER_NODE, startup_frequency_hz);
static bool s_ready;
#endif

int RSCFBuzzerSetTone(uint32_t frequency_hz, uint16_t duty_permille)
{
#if DT_NODE_HAS_STATUS(RSCF_BUZZER_NODE, okay)
  uint32_t period_ns;
  uint32_t pulse_ns;

  if (!s_ready) {
    return -ENODEV;
  }

  if ((frequency_hz == 0U) || (duty_permille > 1000U)) {
    return -EINVAL;
  }

  period_ns = (uint32_t)(NSEC_PER_SEC / frequency_hz);
  pulse_ns = (period_ns * duty_permille) / 1000U;
  return pwm_set_dt(&s_buzzer, period_ns, pulse_ns);
#else
  ARG_UNUSED(frequency_hz);
  ARG_UNUSED(duty_permille);
  return 0;
#endif
}

void RSCFBuzzerStop(void)
{
#if DT_NODE_HAS_STATUS(RSCF_BUZZER_NODE, okay)
  if (s_ready) {
    (void)pwm_set_dt(&s_buzzer, s_buzzer.period, 0U);
  }
#endif
}

int RSCFBuzzerInit(void)
{
#if DT_NODE_HAS_STATUS(RSCF_BUZZER_NODE, okay)
  if (!pwm_is_ready_dt(&s_buzzer)) {
    LOG_WRN("buzzer pwm is not ready");
    s_ready = false;
    return 0;
  }

  s_ready = true;
  (void)RSCFBuzzerSetTone(s_startup_freq_hz, 0U);
  LOG_INF("buzzer initialized");
  return 0;
#else
  LOG_INF("buzzer node disabled");
  return 0;
#endif
}

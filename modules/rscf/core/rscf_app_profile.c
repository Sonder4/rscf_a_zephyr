#include <zephyr/logging/log.h>

#include <rscf/rscf_app_profile.h>
#include <rscf/rscf_event_bus.h>
#include <rscf/rscf_health_service.h>

LOG_MODULE_REGISTER(rscf_profile, LOG_LEVEL_INF);

int RSCFAppProfileInit(void)
{
  int ret;

#if defined(CONFIG_RSCF_EVENT_BUS)
  ret = RSCFEventBusInit();
  if (ret != 0) {
    return ret;
  }
#endif

#if defined(CONFIG_RSCF_HEALTH_SERVICE)
  ret = RSCFHealthServiceInit();
  if (ret != 0) {
    return ret;
  }
#endif

  LOG_INF("profile services initialized");
  return 0;
}

void RSCFAppProfileTick(void)
{
#if defined(CONFIG_RSCF_HEALTH_SERVICE)
  RSCFHealthServiceBeat();
#endif
}

const char *RSCFAppProfileName(void)
{
#if defined(CONFIG_RSCF_PROFILE_FULL)
  return "full";
#elif defined(CONFIG_RSCF_PROFILE_CHASSIS)
  return "chassis";
#elif defined(CONFIG_RSCF_PROFILE_COMM)
  return "comm";
#else
  return "minimal_bringup";
#endif
}

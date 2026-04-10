#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <rscf/rscf_app_profile.h>
#include <rscf/rscf_boot.h>

LOG_MODULE_REGISTER(rscf_main, LOG_LEVEL_INF);

#define RSCF_MAIN_LOOP_PERIOD_MS 10

int main(void)
{
  int ret;

  LOG_INF("RSCF A Zephyr bring-up start");

  ret = RSCFBootRun();
  if (ret != 0) {
    LOG_ERR("boot failed: %d", ret);
    return ret;
  }

  ret = RSCFAppProfileInit();
  if (ret != 0) {
    LOG_ERR("profile init failed: %d", ret);
    return ret;
  }

  LOG_INF("active profile: %s", RSCFAppProfileName());

  while (1) {
    RSCFAppProfileTick();
    k_sleep(K_MSEC(RSCF_MAIN_LOOP_PERIOD_MS));
  }
}

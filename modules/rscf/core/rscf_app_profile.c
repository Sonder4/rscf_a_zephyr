#include <zephyr/logging/log.h>

#include <rscf/rscf_app_profile.h>
#include <rscf/rscf_daemon_service.h>
#include <rscf/rscf_debug_fault.h>
#include <rscf/rscf_event_bus.h>
#include <rscf/rscf_health_service.h>
#include <rscf/rscf_chassis_service.h>
#include <rscf/rscf_motor_service.h>
#include "buzzer.h"
#include "rscf_buzzer.h"
#include "rscf_comm_service.h"
#include "rscf_dtof2510.h"
#include "rscf_ext_uart_mux.h"
#include "rscf_imu_uart.h"
#include "rscf_led_status.h"

LOG_MODULE_REGISTER(rscf_profile, LOG_LEVEL_INF);

int RSCFAppProfileInit(void)
{
  int ret;

#if defined(CONFIG_RSCF_LED_STATUS)
  ret = RSCFLedStatusInit();
  if (ret != 0) {
    return ret;
  }
#endif

#if defined(CONFIG_RSCF_EXT_UART_MUX)
  ret = RSCFExtUartMuxInit();
  if (ret != 0) {
    return ret;
  }
#endif

#if defined(CONFIG_RSCF_BUZZER)
  ret = RSCFBuzzerInit();
  if (ret != 0) {
    return ret;
  }
  BuzzerInit();
#endif

#if defined(CONFIG_RSCF_EVENT_BUS)
  ret = RSCFEventBusInit();
  if (ret != 0) {
    return ret;
  }
#endif

#if defined(CONFIG_RSCF_DAEMON_SERVICE)
  ret = RSCFDaemonServiceInit();
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

#if defined(CONFIG_RSCF_COMM_SERVICE)
  ret = RSCFCommServiceInit();
  if (ret != 0) {
    return ret;
  }
#endif

#if defined(CONFIG_RSCF_MOTOR_SERVICE)
  ret = RSCFMotorServiceInit();
  if (ret != 0) {
    return ret;
  }
#endif

#if defined(CONFIG_RSCF_CHASSIS_SERVICE)
  ret = RSCFChassisServiceInit();
  if (ret != 0) {
    return ret;
  }
#endif

#if defined(CONFIG_RSCF_IMU_UART)
  ret = RSCFImuUartInit();
  if (ret != 0) {
    return ret;
  }
#endif

#if defined(CONFIG_RSCF_DTOF2510)
  ret = RSCFDtof2510Init();
  if (ret != 0) {
    return ret;
  }
#endif

#if defined(CONFIG_RSCF_DEBUG_FAULT)
  ret = RSCFDebugFaultInit();
  if (ret != 0) {
    return ret;
  }
#endif

  LOG_INF("profile services initialized");
  return 0;
}

void RSCFAppProfileTick(void)
{
#if defined(CONFIG_RSCF_LED_STATUS)
  RSCFLedStatusBeat(RSCF_LED_STATUS_BOOT);
  RSCFLedStatusTick();
#endif

#if defined(CONFIG_RSCF_HEALTH_SERVICE)
  RSCFHealthServiceBeat();
#endif

  BuzzerTask();

#if defined(CONFIG_RSCF_COMM_SERVICE)
  RSCFCommServiceProcess();
#endif

#if defined(CONFIG_RSCF_DEBUG_FAULT)
  RSCFDebugFaultPoll();
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

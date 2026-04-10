#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include <rscf/rscf_motor_service.h>

#include "motor_task.h"
#include "servo_motor.h"

LOG_MODULE_REGISTER(rscf_motor_service, LOG_LEVEL_INF);

#define RSCF_MOTOR_CONTROL_PERIOD_MS 1

static struct k_work_delayable s_motor_work;
static bool s_motor_service_ready;

static void RSCFMotorServiceWorkHandler(struct k_work *work)
{
    ARG_UNUSED(work);

    MotorControlTask();
    (void)k_work_reschedule(&s_motor_work, K_MSEC(RSCF_MOTOR_CONTROL_PERIOD_MS));
}

int RSCFMotorServiceInit(void)
{
    if (s_motor_service_ready)
    {
        return 0;
    }

    (void)ServoControlInit();
    k_work_init_delayable(&s_motor_work, RSCFMotorServiceWorkHandler);
    (void)k_work_reschedule(&s_motor_work, K_MSEC(RSCF_MOTOR_CONTROL_PERIOD_MS));
    s_motor_service_ready = true;
    LOG_INF("motor service initialized");
    return 0;
}

bool RSCFMotorServiceReady(void)
{
    return s_motor_service_ready;
}

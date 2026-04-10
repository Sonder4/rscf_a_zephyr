#include "motor_task.h"
#include "servo_motor.h"

void __attribute__((weak)) DJIMotorControl(void)
{
}

void __attribute__((weak)) DMMotorControl(void)
{
}

void __attribute__((weak)) LKMotorControl(void)
{
}

void __attribute__((weak)) HTMotorControl(void)
{
}

void __attribute__((weak)) StepMotorControl(void)
{
}

void __attribute__((weak)) ServoControlTask(void)
{
}

void MotorControlTask(void)
{
    static uint32_t s_motor_tick;

    s_motor_tick++;
    DJIMotorControl();
    if ((s_motor_tick & 0x01U) == 0U)
    {
        DMMotorControl();
    }
    LKMotorControl();
    HTMotorControl();
    StepMotorControl();
    if ((s_motor_tick % SERVO_PWM_CONTROL_TASK_PERIOD_MS) == 0U)
    {
        ServoControlTask();
    }
}

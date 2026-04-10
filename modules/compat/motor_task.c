#include "motor_task.h"

void __attribute__((weak)) DJIMotorControl(void)
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

void MotorControlTask(void)
{
    DJIMotorControl();
    LKMotorControl();
    HTMotorControl();
    StepMotorControl();
}

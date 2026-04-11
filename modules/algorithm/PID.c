#include <stddef.h>
#include <string.h>

#include "PID.h"

static float pid_abs(float value)
{
    return (value < 0.0f) ? -value : value;
}

PID PID_Preview_pid_x = {0};
PID PID_Preview_pid_y = {0};
PID PID_Reflection_pid_x = {0};
PID PID_Reflection_pid_y = {0};
PID PID_Speed_pid_x = {0};
PID PID_Speed_pid_y = {0};
PID PID_OpenCV_relative_x = {0};
PID PID_OpenCV_relative_y = {0};

float PID_count(PID *pid, float now, float set)
{
    if (pid == NULL)
    {
        return 0.0f;
    }

    pid->error_this_time = set - now;
    if ((pid->dead_zone_flag == 1) && (pid_abs(pid->error_this_time) <= pid->dead_zone))
    {
        pid->error_this_time = 0.0f;
    }

    pid->error_sum += pid->error_this_time;
    if (pid->I_limit_flag == 1)
    {
        if (pid->error_sum > pid->I_SUM_limit)
        {
            pid->error_sum = pid->I_SUM_limit;
        }
        else if (pid->error_sum < -pid->I_SUM_limit)
        {
            pid->error_sum = -pid->I_SUM_limit;
        }
    }

    pid->output = pid->KP * pid->error_this_time +
                  pid->KI * pid->error_sum +
                  pid->KD * (pid->error_this_time - pid->error_last_time) +
                  pid->KF * set;

    if (pid->OUTPUT_limit_flag == 1)
    {
        if (pid->output > pid->OUTPUT_limit)
        {
            pid->output = pid->OUTPUT_limit;
        }
        else if (pid->output < -pid->OUTPUT_limit)
        {
            pid->output = -pid->OUTPUT_limit;
        }
    }

    pid->error_last_time = pid->error_this_time;
    return pid->output;
}

void PID_parameter_setting(PID *pid,
                           float kp,
                           float ki,
                           float kd,
                           float i_limit,
                           int i_limit_flag,
                           float output_limit,
                           int output_limit_flag,
                           float dead_zone,
                           int dead_zone_flag)
{
    if (pid == NULL)
    {
        return;
    }

    pid->KP = kp;
    pid->KI = ki;
    pid->KD = kd;
    pid->I_SUM_limit = i_limit;
    pid->I_limit_flag = i_limit_flag;
    pid->OUTPUT_limit = output_limit;
    pid->OUTPUT_limit_flag = output_limit_flag;
    pid->dead_zone = dead_zone;
    pid->dead_zone_flag = dead_zone_flag;
}

void PID_reset(PID *pid)
{
    if (pid == NULL)
    {
        return;
    }

    memset(&pid->error_this_time, 0, sizeof(PID) - offsetof(PID, error_this_time));
}

#ifndef PID_H_
#define PID_H_

#include "arm_math.h"

typedef struct
{
    float KP;
    float KI;
    float KD;
    float KF;
    float error_this_time;
    float error_last_time;
    float error_sum;
    float I_SUM_limit;
    float OUTPUT_limit;
    int I_limit_flag;
    int OUTPUT_limit_flag;
    float output;
    float dead_zone;
    int dead_zone_flag;
} PID;

extern PID PID_Preview_pid_x;
extern PID PID_Preview_pid_y;
extern PID PID_Reflection_pid_x;
extern PID PID_Reflection_pid_y;
extern PID PID_Speed_pid_x;
extern PID PID_Speed_pid_y;
extern PID PID_OpenCV_relative_x;
extern PID PID_OpenCV_relative_y;

float PID_count(PID *pid, float now, float set);
void PID_parameter_setting(
    PID *pid,
    float kp,
    float ki,
    float kd,
    float i_limit,
    int i_limit_flag,
    float output_limit,
    int output_limit_flag,
    float dead_zone,
    int dead_zone_flag
);
void PID_reset(PID *pid);

#endif /* PID_H_ */

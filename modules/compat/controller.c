#include <string.h>

#include "controller.h"

#define PID_FABS(x) ((x) >= 0.0f ? (x) : -(x))

static void f_Trapezoid_Intergral(PIDInstance *pid)
{
    pid->ITerm = pid->Ki * ((pid->Err + pid->Last_Err) * 0.5f) * pid->dt;
}

static void f_Changing_Integration_Rate(PIDInstance *pid)
{
    if (pid->Err * pid->Iout > 0.0f)
    {
        float abs_err = PID_FABS(pid->Err);

        if (abs_err <= pid->CoefB)
        {
            return;
        }
        if (abs_err <= (pid->CoefA + pid->CoefB))
        {
            pid->ITerm *= (pid->CoefA - abs_err + pid->CoefB) / pid->CoefA;
        }
        else
        {
            pid->ITerm = 0.0f;
        }
    }
}

static void f_Integral_Limit(PIDInstance *pid)
{
    float temp_iout = pid->Iout + pid->ITerm;
    float temp_output = pid->Pout + temp_iout + pid->Dout;

    if ((PID_FABS(temp_output) > pid->MaxOut) && (pid->Err * pid->Iout > 0.0f))
    {
        pid->ITerm = 0.0f;
        temp_iout = pid->Iout;
    }

    if (temp_iout > pid->IntegralLimit)
    {
        pid->ITerm = 0.0f;
        pid->Iout = pid->IntegralLimit;
    }
    else if (temp_iout < -pid->IntegralLimit)
    {
        pid->ITerm = 0.0f;
        pid->Iout = -pid->IntegralLimit;
    }
}

static void f_Derivative_On_Measurement(PIDInstance *pid)
{
    pid->Dout = pid->Kd * (pid->Last_Measure - pid->Measure) / pid->dt;
}

static void f_Derivative_Filter(PIDInstance *pid)
{
    pid->Dout = pid->Dout * pid->dt / (pid->Derivative_LPF_RC + pid->dt) +
                pid->Last_Dout * pid->Derivative_LPF_RC / (pid->Derivative_LPF_RC + pid->dt);
}

static void f_Output_Filter(PIDInstance *pid)
{
    pid->Output = pid->Output * pid->dt / (pid->Output_LPF_RC + pid->dt) +
                  pid->Last_Output * pid->Output_LPF_RC / (pid->Output_LPF_RC + pid->dt);
}

static void f_Output_Limit(PIDInstance *pid)
{
    if (pid->Output > pid->MaxOut)
    {
        pid->Output = pid->MaxOut;
    }
    else if (pid->Output < -pid->MaxOut)
    {
        pid->Output = -pid->MaxOut;
    }
}

static void f_PID_ErrorHandle(PIDInstance *pid)
{
    if ((PID_FABS(pid->Output) < pid->MaxOut * 0.001f) || (PID_FABS(pid->Ref) < 0.0001f))
    {
        return;
    }

    if ((PID_FABS(pid->Ref - pid->Measure) / PID_FABS(pid->Ref)) > 0.95f)
    {
        pid->ERRORHandler.ERRORCount++;
    }
    else
    {
        pid->ERRORHandler.ERRORCount = 0U;
    }

    if (pid->ERRORHandler.ERRORCount > 500U)
    {
        pid->ERRORHandler.ERRORType = PID_MOTOR_BLOCKED_ERROR;
    }
}

void PIDInit(PIDInstance *pid, PID_Init_Config_s *config)
{
    if ((pid == NULL) || (config == NULL))
    {
        return;
    }

    memset(pid, 0, sizeof(PIDInstance));
    memcpy(pid, config, sizeof(PID_Init_Config_s));
    DWT_GetDeltaT(&pid->DWT_CNT);
}

float PIDCalculate(PIDInstance *pid, float measure, float ref)
{
    if (pid == NULL)
    {
        return 0.0f;
    }

    if (pid->Improve & PID_ErrorHandle)
    {
        f_PID_ErrorHandle(pid);
    }

    pid->dt = DWT_GetDeltaT(&pid->DWT_CNT);
    if (pid->dt <= 0.0f)
    {
        pid->dt = 1e-6f;
    }

    pid->Measure = measure;
    pid->Ref = ref;
    pid->Err = pid->Ref - pid->Measure;

    if (PID_FABS(pid->Err) > pid->DeadBand)
    {
        pid->Pout = pid->Kp * pid->Err;
        pid->ITerm = pid->Ki * pid->Err * pid->dt;
        pid->Dout = pid->Kd * (pid->Err - pid->Last_Err) / pid->dt;

        if (pid->Improve & PID_Trapezoid_Intergral)
        {
            f_Trapezoid_Intergral(pid);
        }
        if (pid->Improve & PID_ChangingIntegrationRate)
        {
            f_Changing_Integration_Rate(pid);
        }
        if (pid->Improve & PID_Derivative_On_Measurement)
        {
            f_Derivative_On_Measurement(pid);
        }
        if (pid->Improve & PID_DerivativeFilter)
        {
            f_Derivative_Filter(pid);
        }
        if (pid->Improve & PID_Integral_Limit)
        {
            f_Integral_Limit(pid);
        }

        pid->Iout += pid->ITerm;
        pid->Output = pid->Pout + pid->Iout + pid->Dout;

        if (pid->Improve & PID_OutputFilter)
        {
            f_Output_Filter(pid);
        }

        f_Output_Limit(pid);
    }
    else
    {
        pid->Output = 0.0f;
        pid->ITerm = 0.0f;
    }

    pid->Last_Measure = pid->Measure;
    pid->Last_Output = pid->Output;
    pid->Last_Dout = pid->Dout;
    pid->Last_Err = pid->Err;
    pid->Last_ITerm = pid->ITerm;

    return pid->Output;
}

/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

*******************************************************************************
  文 件 名: unit_convert.c
  版 本 号: V20260406.1
  作    者: Xuan
  生成日期: 2026-04-06
  功能描述: 统一单位换算与一阶低通滤波实现
  补    充: 默认物理参数以当前工程使用型号为准，后续可按实测修订

*******************************************************************************/

#include <stddef.h>

#include "unit_convert.h"
#include "general_def.h"

float UnitDegToRad(float deg)
{
    return deg * RAD_PER_DEG;
}

float UnitRadToDeg(float rad)
{
    return rad * DEG_PER_RAD;
}

float UnitRpmToRadps(float rpm)
{
    return rpm * RPM_2_RAD_PER_SEC;
}

float UnitRadpsToRpm(float radps)
{
    return radps * RAD_PER_SEC_2_RPM;
}

float UnitMmToM(float mm)
{
    return mm * 0.001f;
}

float UnitMToMm(float m)
{
    return m * 1000.0f;
}

float UnitMmpsToMps(float mmps)
{
    return mmps * 0.001f;
}

float UnitMpsToMmps(float mps)
{
    return mps * 1000.0f;
}

float UnitGToMps2(float g)
{
    return g * MPS2_PER_G;
}

float UnitDpsToRadps(float dps)
{
    return dps * RAD_PER_DEG;
}

float UnitRotorToOutputPosRad(float rotor_pos_rad, float ratio)
{
    if (ratio <= 0.0f)
    {
        return rotor_pos_rad;
    }

    return rotor_pos_rad / ratio;
}

float UnitRotorToOutputVelRadps(float rotor_vel_radps, float ratio)
{
    if (ratio <= 0.0f)
    {
        return rotor_vel_radps;
    }

    return rotor_vel_radps / ratio;
}

float UnitOutputToWheelMps(float out_vel_radps, float wheel_radius_m)
{
    return out_vel_radps * wheel_radius_m;
}

float UnitRawCurrentToMa(float raw_current, float cur_ma_per_raw)
{
    return raw_current * cur_ma_per_raw;
}

float UnitCurrentMaToTorqueNm(float cur_ma, float tor_nm_per_ma)
{
    return cur_ma * tor_nm_per_ma;
}

void UnitLpf1Init(UnitLpf1_t *filter, float alpha)
{
    if (filter == NULL)
    {
        return;
    }

    if (alpha < 0.0f)
    {
        alpha = 0.0f;
    }
    else if (alpha > 1.0f)
    {
        alpha = 1.0f;
    }

    filter->alpha = alpha;
    filter->y = 0.0f;
    filter->inited = 0U;
}

float UnitLpf1Update(UnitLpf1_t *filter, float sample)
{
    if (filter == NULL)
    {
        return sample;
    }

    if (filter->inited == 0U)
    {
        filter->y = sample;
        filter->inited = 1U;
        return filter->y;
    }

    filter->y = (1.0f - filter->alpha) * filter->y + filter->alpha * sample;
    return filter->y;
}

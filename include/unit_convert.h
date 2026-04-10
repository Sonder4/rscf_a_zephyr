/*******************************************************************************

                      版权所有 (C), 2025-2026, NCU_Roboteam

*******************************************************************************
  文 件 名: unit_convert.h
  版 本 号: V20260406.1
  作    者: Xuan
  生成日期: 2026-04-06
  功能描述: 统一单位换算与一阶低通滤波接口
  补    充: 提供角度/速度/线速度/电流/力矩等通用物理量换算与一阶滤波

*******************************************************************************/

#ifndef UNIT_CONVERT_H
#define UNIT_CONVERT_H

#include <stdint.h>

typedef struct
{
    float alpha;
    float y;
    uint8_t inited;
} UnitLpf1_t;

float UnitDegToRad(float deg);
float UnitRadToDeg(float rad);
float UnitRpmToRadps(float rpm);
float UnitRadpsToRpm(float radps);
float UnitMmToM(float mm);
float UnitMToMm(float m);
float UnitMmpsToMps(float mmps);
float UnitMpsToMmps(float mps);
float UnitGToMps2(float g);
float UnitDpsToRadps(float dps);
float UnitRotorToOutputPosRad(float rotor_pos_rad, float ratio);
float UnitRotorToOutputVelRadps(float rotor_vel_radps, float ratio);
float UnitOutputToWheelMps(float out_vel_radps, float wheel_radius_m);
float UnitRawCurrentToMa(float raw_current, float cur_ma_per_raw);
float UnitCurrentMaToTorqueNm(float cur_ma, float tor_nm_per_ma);
void UnitLpf1Init(UnitLpf1_t *filter, float alpha);
float UnitLpf1Update(UnitLpf1_t *filter, float sample);

#endif /* UNIT_CONVERT_H */

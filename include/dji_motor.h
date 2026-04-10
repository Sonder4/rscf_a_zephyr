#ifndef DJI_MOTOR_H
#define DJI_MOTOR_H

#include <stdint.h>

#include "bsp_can.h"
#include "controller.h"
#include "daemon.h"
#include "motor_def.h"
#include "unit_convert.h"

#define DJI_MOTOR_CNT 12
#define SPEED_SMOOTH_COEF 0.85f
#define CURRENT_SMOOTH_COEF 0.9f

typedef struct
{
    uint16_t last_ecd;
    uint16_t ecd;
    int16_t speed_rpm;
    int16_t real_current;
    uint8_t temperature;
} DJI_Motor_Measure_s;

typedef struct
{
    DJI_Motor_Measure_s measure;
    MotorFb_t fb;
    MotorPhyCfg_t phy_cfg;
    Motor_Control_Setting_s motor_settings;
    Motor_Controller_s motor_controller;
    CANInstance *motor_can_instance;
    uint8_t sender_group;
    uint8_t message_num;
    Motor_Type_e motor_type;
    Motor_Working_Type_e stop_flag;
    DaemonInstance *daemon;
    uint32_t feed_cnt;
    float dt;
    int32_t total_round;
    UnitLpf1_t vel_lpf;
    UnitLpf1_t cur_lpf;
    UnitLpf1_t tor_lpf;
} DJIMotorInstance;

float DJIMotorGetDefaultRatio(Motor_Type_e motor_type);
float DJIMotorGetDefaultCurMaPerRaw(Motor_Type_e motor_type);
float DJIMotorGetDefaultTorNmPerMa(Motor_Type_e motor_type);
void DJIMotorNormalizePhyCfg(Motor_Type_e motor_type, MotorPhyCfg_t *phy_cfg);
DJIMotorInstance *DJIMotorInit(Motor_Init_Config_s *config);
void DJIMotorSetRef(DJIMotorInstance *motor, float ref);
void DJIMotorChangeFeed(DJIMotorInstance *motor, Closeloop_Type_e loop, Feedback_Source_e type);
void DJIMotorControl(void);
void DJIMotorStop(DJIMotorInstance *motor);
void DJIMotorEnable(DJIMotorInstance *motor);
void DJIMotorOuterLoop(DJIMotorInstance *motor, Closeloop_Type_e outer_loop);
void DJIMotorSetFreq(DJIMotorInstance *motor, Motor_Control_Freq_e freq);

#endif /* DJI_MOTOR_H */

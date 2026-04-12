#ifndef RSCF_CHASSIS_SERVICE_H_
#define RSCF_CHASSIS_SERVICE_H_

#include <stdbool.h>
#include <stdint.h>

int RSCFChassisServiceInit(void);
void RSCFChassisServiceTick(void);
void RSCFChassisServiceSetEnable(bool enable);
void RSCFChassisServiceStop(void);
void RSCFChassisServiceSetTarget(float vx_mps, float vy_mps, float yaw_rate_degps);
void RSCFChassisServiceSetControlMode(uint8_t ctrl_mode);
void RSCFChassisServiceSetHeadingMode(uint8_t heading_mode);
void RSCFChassisServiceSetVelocityFrame(uint8_t velocity_frame);
void RSCFChassisServiceSetHeadingTarget(float heading_deg);
uint8_t RSCFChassisServiceGetControlMode(void);
uint8_t RSCFChassisServiceGetHeadingMode(void);
uint8_t RSCFChassisServiceGetVelocityFrame(void);
bool RSCFChassisServiceGetOdom(float *x_m,
                               float *y_m,
                               float *yaw_rad,
                               float *vx_mps,
                               float *vy_mps,
                               float *wz_radps);
bool RSCFChassisServiceGetImuData(float *roll_deg,
                                  float *pitch_deg,
                                  float *yaw_deg,
                                  float *gyro_z_dps,
                                  float *acc_x,
                                  float *acc_y,
                                  float *acc_z);
bool RSCFChassisServiceAnyMotorOnline(void);
bool RSCFChassisServiceAllMotorsOnline(void);

#endif /* RSCF_CHASSIS_SERVICE_H_ */

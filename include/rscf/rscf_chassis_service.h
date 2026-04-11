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
uint8_t RSCFChassisServiceGetControlMode(void);
uint8_t RSCFChassisServiceGetHeadingMode(void);
uint8_t RSCFChassisServiceGetVelocityFrame(void);
bool RSCFChassisServiceAnyMotorOnline(void);
bool RSCFChassisServiceAllMotorsOnline(void);

#endif /* RSCF_CHASSIS_SERVICE_H_ */

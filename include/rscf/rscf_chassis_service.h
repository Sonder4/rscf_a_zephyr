#ifndef RSCF_CHASSIS_SERVICE_H_
#define RSCF_CHASSIS_SERVICE_H_

#include <stdbool.h>
#include <stdint.h>

int RSCFChassisServiceInit(void);
void RSCFChassisServiceSetEnable(bool enable);
void RSCFChassisServiceStop(void);
void RSCFChassisServiceSetTarget(float vx_mps, float vy_mps, float yaw_rate_degps);
bool RSCFChassisServiceAnyMotorOnline(void);
bool RSCFChassisServiceAllMotorsOnline(void);

#endif /* RSCF_CHASSIS_SERVICE_H_ */

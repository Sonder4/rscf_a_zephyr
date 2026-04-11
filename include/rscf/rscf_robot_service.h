#ifndef RSCF_ROBOT_SERVICE_H_
#define RSCF_ROBOT_SERVICE_H_

#include <stdbool.h>

int RSCFRobotServiceInit(void);
void RSCFRobotServiceTick(void);
bool RSCFRobotServiceReady(void);

#endif /* RSCF_ROBOT_SERVICE_H_ */

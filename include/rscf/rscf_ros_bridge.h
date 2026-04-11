#ifndef RSCF_ROS_BRIDGE_H_
#define RSCF_ROS_BRIDGE_H_

#include <stdbool.h>

typedef enum
{
    RSCF_ROS_BACKEND_MCU_COMM = 0,
    RSCF_ROS_BACKEND_MICROROS = 1
} RSCFRosBackend_e;

int RSCFRosBridgeInit(void);
void RSCFRosBridgeSpinOnce(void);
bool RSCFRosBridgeReady(void);
RSCFRosBackend_e RSCFRosBridgeBackend(void);
const char *RSCFRosBridgeTransportName(void);

#endif /* RSCF_ROS_BRIDGE_H_ */

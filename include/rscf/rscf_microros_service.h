#ifndef RSCF_MICROROS_SERVICE_H_
#define RSCF_MICROROS_SERVICE_H_

#include <stdbool.h>

int RSCFMicroRosServiceInit(void);
void RSCFMicroRosServiceTick(void);
bool RSCFMicroRosServiceReady(void);
bool RSCFMicroRosServiceConnected(void);

#endif /* RSCF_MICROROS_SERVICE_H_ */

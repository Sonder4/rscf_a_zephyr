#ifndef RSCF_COMM_SERVICE_H_
#define RSCF_COMM_SERVICE_H_

#include <stdbool.h>

int RSCFCommServiceInit(void);
void RSCFCommServiceProcess(void);
bool RSCFCommServiceReady(void);

#endif /* RSCF_COMM_SERVICE_H_ */

#ifndef RSCF_LINK_ADAPTER_H_
#define RSCF_LINK_ADAPTER_H_

#include <stdbool.h>

#include "transport_interface.h"

struct rscf_link_adapter {
  const char *name;
  const char *role;
  Transport_interface_t *transport;
  bool (*is_ready)(void);
};

const struct rscf_link_adapter *RSCFLinkUsbAdapterGet(void);
const struct rscf_link_adapter *RSCFLinkUartAdapterGet(void);
const struct rscf_link_adapter *RSCFLinkCanAdapterGet(void);
const struct rscf_link_adapter *RSCFLinkSpiAdapterGet(void);

#endif /* RSCF_LINK_ADAPTER_H_ */

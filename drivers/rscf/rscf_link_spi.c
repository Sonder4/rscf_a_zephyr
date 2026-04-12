#include "rscf_link_adapter.h"

static bool rscf_link_spi_is_ready(void)
{
  return false;
}

static const struct rscf_link_adapter s_spi_adapter = {
  .name = "spi",
  .role = "peer",
  .transport = NULL,
  .is_ready = rscf_link_spi_is_ready,
};

const struct rscf_link_adapter *RSCFLinkSpiAdapterGet(void)
{
  return &s_spi_adapter;
}

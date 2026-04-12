#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include "rscf_link_adapter.h"
#include "transport_usb.h"

static bool rscf_link_usb_is_ready(void)
{
#if DT_NODE_EXISTS(DT_CHOSEN(rscf_host_link)) && DT_NODE_HAS_STATUS(DT_CHOSEN(rscf_host_link), okay)
  const struct device *host_usb = DEVICE_DT_GET(DT_CHOSEN(rscf_host_link));

  return device_is_ready(host_usb);
#else
  return false;
#endif
}

static const struct rscf_link_adapter s_usb_adapter = {
  .name = "usb",
  .role = "host",
  .transport = &g_usb_transport,
  .is_ready = rscf_link_usb_is_ready,
};

const struct rscf_link_adapter *RSCFLinkUsbAdapterGet(void)
{
  return &s_usb_adapter;
}

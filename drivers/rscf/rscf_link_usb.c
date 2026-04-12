#include <zephyr/device.h>
#include <zephyr/devicetree.h>

#include "transport_usb.h"

Transport_interface_t *RSCFLinkUsbAdapterGetTransport(void)
{
  return &g_usb_transport;
}

bool RSCFLinkUsbAdapterIsReady(void)
{
#if DT_NODE_EXISTS(DT_CHOSEN(rscf_host_link)) && DT_NODE_HAS_STATUS(DT_CHOSEN(rscf_host_link), okay)
  const struct device *host_usb = DEVICE_DT_GET(DT_CHOSEN(rscf_host_link));

  return device_is_ready(host_usb);
#else
  return false;
#endif
}

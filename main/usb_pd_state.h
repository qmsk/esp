#pragma once

#include <sdkconfig.h>

#if CONFIG_USB_PD_SINK_ENABLED
  #include <usb_pd_sink.h>

  extern struct usb_pd_sink *usb_pd_sink;
#endif

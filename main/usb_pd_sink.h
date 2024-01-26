#pragma once

#include <sdkconfig.h>

#if CONFIG_USB_PD_SINK_ENABLED
  int init_usb_pd_sink();
  int start_usb_pd_sink();
#endif

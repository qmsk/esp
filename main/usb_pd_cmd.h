#pragma once

#include <cmd.h>
#include <sdkconfig.h>

#if CONFIG_USB_PD_SINK_ENABLED
  extern const struct cmdtab usb_pd_cmdtab;
#endif

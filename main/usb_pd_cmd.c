#include "usb_pd_cmd.h"
#include "usb_pd_state.h"

#include <logging.h>

#include <stdio.h>

#if CONFIG_USB_PD_SINK_ENABLED
  int usb_pd_sink_status_cmd(int argc, char **argv, void *ctx)
  {
    struct usb_pd_sink_status status;
    int err;

    if (!usb_pd_sink) {
      LOG_ERROR("not initialized");
      return -1;
    }

    if ((err = usb_pd_sink_status(usb_pd_sink, &status))) {
      LOG_ERROR("usb_pd_sink_status");
      return -1;
    }

    printf("USB-PD Version %d.%d\n", status.usb_pd_version_major, status.usb_pd_version_minor);
    printf("Sink:\n");
    printf("\t%-25s: %d\n", "Port Attached", status.port_attached);
    printf("\t%-25s: %d\n", "Port Powered", status.port_power);
    printf("\t%-25s: %d\n", "VBUS Ready", status.vbus_ready);

    printf("\t%-25s: %d\n", "PDO #", status.pdo_number);
    printf("\t%-25s: %d\n", "PD Capability Mismatch", status.pd_capability_mismatch);
    printf("\t%-25s: %.2f\n", "Voltage", status.voltage_mV / 1000.0f);
    printf("\t%-25s: %.2f - %.2f\n", "Current", status.operating_current_mA / 1000.0f, status.maximum_current_mA / 1000.0f);

    return 0;
  }

  const struct cmd usb_pd_commands[] = {
    { "status",   usb_pd_sink_status_cmd,  .describe = "Show USB-PD Sink Status"  },
    {}
  };

  const struct cmdtab usb_pd_cmdtab = {
    .commands = usb_pd_commands,
  };
#endif

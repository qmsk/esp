#include "usb_pd_sink.h"
#include "i2c_master.h"

#include <usb_pd_sink.h>

#include <logging.h>

#if CONFIG_USB_PD_SINK_ENABLED
  struct usb_pd_sink *usb_pd_sink;

  int init_usb_pd_sink()
  {
    struct usb_pd_sink_options options = {
      #if CONFIG_USB_PD_SINK_TYPE_STUSB4500
        .type     = USB_PD_SINK_STUSB4500,
        .i2c_port = I2C_MASTER_PORT,
        .i2c_addr = CONFIG_USB_PD_SINK_TYPE_STUSB4500_I2C_ADDR,
      #else
        #error "No USB-PD Sink Type selected"
      #endif
    };
    int err;

    LOG_INFO("type=%d i2c_port=%d i2c_addr=%d", options.type, options.i2c_port, options.i2c_addr);

    if ((err = usb_pd_sink_new(&usb_pd_sink, &options))) {
      LOG_ERROR("usb_pd_sink_new");
      return err;
    }

    return 0;
  }

  int start_usb_pd_sink()
  {
    int err;

    if ((err = usb_pd_sink_start(usb_pd_sink))) {
      LOG_ERROR("usb_pd_sink_start");
      return err;
    }

    return 0;
  }
#endif

#pragma once

#include <usb_pd_sink.h>

struct stusb4500 {
  i2c_port_t i2c_port;
  uint8_t i2c_addr;
  TickType_t i2c_timeout;
};

int stusb4500_init(struct stusb4500 *stusb, const struct usb_pd_sink_options *options);
int stusb4500_start(struct stusb4500 *stusb);

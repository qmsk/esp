#pragma once

#include "stusb4500_i2c.h"
#include "stusb4500_nvm.h"

#include <usb_pd_sink.h>

struct stusb4500 {
  i2c_port_t i2c_port;
  uint8_t i2c_addr;
  TickType_t i2c_timeout;
};

int stusb4500_init(struct stusb4500 *stusb4500, const struct usb_pd_sink_options *options);
int stusb4500_reset(struct stusb4500 *stusb4500);
int stusb4500_setup(struct stusb4500 *stusb4500);
int stusb4500_start(struct stusb4500 *stusb4500);
int stusb4500_status(struct stusb4500 *stusb4500, struct usb_pd_sink_status *status);

int stusb4500_i2c_read(struct stusb4500 *stusb4500, enum stusb4500_i2c_register reg, void *buf, size_t size);
int stusb4500_i2c_write(struct stusb4500 *stusb4500, enum stusb4500_i2c_register reg, const void *buf, size_t size);

int stusb4500_nvm_read(struct stusb4500 *stusb4500, union stusb4500_nvm *nvm);
int stusb4500_nvm_write(struct stusb4500 *stusb4500, const union stusb4500_nvm *nvm);

/* Return 0 if unchanged, >0 if modified, <0 on errors */
int stusb4500_config_nvm(union stusb4500_nvm *nvm);

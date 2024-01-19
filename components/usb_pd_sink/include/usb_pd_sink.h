#pragma once

#include <freertos/FreeRTOS.h>
#include <hal/i2c_types.h>

#define USB_PD_SINK_STUSB4500_I2C_ADDR_BASE 0x28
#define USB_PD_SINK_STUSB4500_I2C_ADDR_MASK 0x03
#define USB_PD_SINK_STUSB4500_I2C_ADDR(addr) (USB_PD_SINK_STUSB4500_I2C_ADDR_BASE | ((addr) & USB_PD_SINK_STUSB4500_I2C_ADDR_MASK))
#define USB_PD_SINK_STUSB4500_I2C_TIMEOUT (1000 / portTICK_RATE_MS)

enum usb_pd_sink_type {
  USB_PD_SINK_STUSB4500,
};

struct usb_pd_sink_options {
  enum usb_pd_sink_type type;

  i2c_port_t i2c_port;
  uint8_t i2c_addr;
  // gpio_pin_t int_pin;

};

struct usb_pd_sink;

int usb_pd_sink_new(struct usb_pd_sink **sinkp, const struct usb_pd_sink_options *options);

/*
 * Static USB-PD Sink setup from Kconfig.
 */
int usb_pd_sink_setup(struct usb_pd_sink *sink);

/*
 * Start USB-PD Sink.
 */
int usb_pd_sink_start(struct usb_pd_sink *sink);

#pragma once

#include <freertos/FreeRTOS.h>
#include <hal/i2c_types.h>
#include <stdio.h>

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

struct usb_pd_sink_status {
  uint8_t usb_pd_version_major;
  uint8_t usb_pd_version_minor;

  bool port_attached;
  bool port_power;              // USB-PD Sink active
  bool vbus_ready;              // VBUS at 5V or higher negotiated power level

  uint8_t pdo_number; // negotiated source PDO, 0 if no USB-PD
  bool pd_capability_mismatch;  // failed to match source/sink PDO

  unsigned voltage_mV;  // negotiated voltage in mV
  unsigned operating_current_mA;  // negotiated operating current in mA
  unsigned maximum_current_mA;    // negotiated maximum current in mA
};

struct usb_pd_sink;

int usb_pd_sink_new(struct usb_pd_sink **sinkp, const struct usb_pd_sink_options *options);

/*
 * Setup USB-PD Sink using static Kconfig defaults.
 */
int usb_pd_sink_setup(struct usb_pd_sink *sink);

/*
 * Start USB-PD Sink.
 */
int usb_pd_sink_start(struct usb_pd_sink *sink);

/*
 * Get generic USB-PD Sink info.
 */
int usb_pd_sink_status(struct usb_pd_sink *sink, struct usb_pd_sink_status *status);

/*
 * Print detailed USB-PD Sink info.
 */
int usb_pd_sink_print(struct usb_pd_sink *sink, FILE *file);

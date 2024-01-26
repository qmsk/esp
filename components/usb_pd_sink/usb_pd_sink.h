#pragma once

#include <usb_pd_sink.h>

#include "stusb4500.h"

struct usb_pd_sink {
  enum usb_pd_sink_type type;

  union {
    struct stusb4500 stusb4500;
  } state;
};

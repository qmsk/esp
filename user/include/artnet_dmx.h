#ifndef __ARTNET_IO_H__
#define __ARTNET_IO_H__

#include "artnet.h"

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define ARTNET_DMX_SIZE 512

struct artnet_dmx {
  uint16_t len;
  uint8_t data[ARTNET_DMX_SIZE];
};

#endif

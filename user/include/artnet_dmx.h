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

/** Patch output port to send on queue.
 *
 * @param addr Art-Net universe address, must match the config.artnet.universe + 0..16
 * @param queue of struct artnet_dmx, written from the artnet task
 *
 * XXX: uses xQueueOverwrite to keep the latest packet, queue MUST be of size 1
 */
int start_artnet_output(uint16_t addr, xQueueHandle queue);

#endif

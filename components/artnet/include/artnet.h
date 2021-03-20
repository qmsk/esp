#ifndef __ARTNET_H__
#define __ARTNET_H__

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#define ARTNET_PORT 6454
#define ARTNET_DMX_SIZE 512

struct artnet;

struct artnet_options {
  // UDP used for listen()
  uint16_t port;

  // all output ports must be within the same sub-net (lower 4 bits)
  uint16_t universe;

  // only used for poll reply, not listen()
  uint8_t ip_address[4];
  uint8_t mac_address[6];
  char short_name[18]; // max 17 chars
  char long_name[64]; // max 63 chars
};

struct artnet_dmx {
  uint16_t len;
  uint8_t data[ARTNET_DMX_SIZE];
};

int artnet_new(struct artnet **artnetp, struct artnet_options options);

/** Patch output port.
 *
 * artnet supports a maximum of 4 output ports with addresses in the artnet_options.universe subnet, i.e. only the lower 4 bits can vary across ports.
 *
 * @param artnet
 * @param addr Art-Net universe address, upper bits must match artnet_options.universe & 0xfff0
 * @param output struct artnet_dmx queue of size 1, written from the artnet task
 *
 * NOT concurrent-safe, must be called between artnet_new() and artnet_start()!
 */
int artnet_add_output(struct artnet *artnet, uint16_t addr, xQueueHandle queue);

/** Start artnet task.
 */
int artnet_start(struct artnet *artnet);

#endif

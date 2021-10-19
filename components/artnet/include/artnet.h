#ifndef __ARTNET_H__
#define __ARTNET_H__

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>

#define ARTNET_PORT 6454
#define ARTNET_DMX_SIZE 512

#define ARTNET_NET_MAX 127
#define ARTNET_SUBNET_MAX 15
#define ARTNET_UNIVERSE_MAX 15

struct artnet;

struct artnet_options {
  // UDP used for listen()
  uint16_t port;

  // all output ports must be within the same sub-net (lower 4 bits)
  uint16_t address;

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

/*
 * Pack artnet address from net + subnet + uni.
 */
uint16_t artnet_address(uint16_t net, uint16_t subnet, uint16_t uni);
uint16_t artnet_address_net(uint16_t address);
uint16_t artnet_address_subnet(uint16_t address);
uint16_t artnet_address_universe(uint16_t address);

int artnet_new(struct artnet **artnetp, struct artnet_options options);

/**
 * Return options used for artnet discovery.
 */
struct artnet_options artnet_get_options(struct artnet *artnet);

/**
 * Update options for artnet discovery.
 *
 * NOTE: changing `port` will not work.
 */
int artnet_set_options(struct artnet *artnet, struct artnet_options options);

/** Patch single output port.
 *
 * Only four output ports are supported.
 * All output port addresses must use an output universe matching the artnet_options.universe subnet, i.e. only the lower 4 bits can vary across ports.
 *
 * @param artnet
 * @param addr Art-Net universe address, upper bits must match artnet_options.universe & 0xfff0
 * @param queue `struct artnet_dmx` queue of size 1, overwritten from the artnet task
 *
 * NOT concurrent-safe, must be called between artnet_new() and artnet_main()!
 */
int artnet_add_output(struct artnet *artnet, uint16_t addr, xQueueHandle queue);

/** Patch multiple output ports.
 *
 * Up to 16 total output ports are supported.
 * All output port addresses must use an output universe matching the artnet_options.universe subnet, i.e. only the lower 4 bits can vary across ports.
 *
 * With multiple outputs, a direct-to-task notification with a bit matching the output index will be sent for each queue write.
 *
 * @param artnet
 * @param address Art-Net universe address, upper bits must match artnet_options.universe & 0xfff0
 * @param index differentiate between multiple outputs for a task
 * @param queue `struct artnet_dmx` queue of size 1, overwritten from the artnet task
 * @param task send direct-to-task notification for queue writes
 *
 * NOT concurrent-safe, must be called between artnet_new() and artnet_main()!
 */
int artnet_add_outputs(struct artnet *artnet, uint16_t address, uint8_t index, xQueueHandle queue, xTaskHandle task);

/** Run artnet mainloop.
 *
 * Logs warnings for protocol errors.
 *
 * Returns on network error.
 */
int artnet_main(struct artnet *artnet);

#endif

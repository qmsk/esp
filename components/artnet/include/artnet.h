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

// up to 16 task notification bits for indexed outputs
#define ARTNET_OUTPUT_TASK_INDEX_BITS 0xffff
#define ARTNET_OUTPUT_TASK_FLAG_BITS 0xffff0000

// flag bit for output sync
#define ARTNET_OUTPUT_TASK_SYNC_BIT 0x10000

// flag bit for output test
#define ARTNET_OUTPUT_TASK_TEST_BIT 0x20000

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
  // flags
  uint8_t sync_mode : 1; // receiver is in sync mode, wait for sync event before refreshing output

  // received sequence number
  uint8_t seq : 8;

  // data length
  uint16_t len;

  // data bytes
  uint8_t data[ARTNET_DMX_SIZE];
};

enum artnet_output_port {
  ARTNET_OUTPUT_PORT_1,
  ARTNET_OUTPUT_PORT_2,
  ARTNET_OUTPUT_PORT_3,
  ARTNET_OUTPUT_PORT_4,
};

struct artnet_output_options {
  /* ArtNet physical output port 1-4 */
  enum artnet_output_port port;

  /* Output index, used for discovery bind index and task notification bits */
  uint8_t index;

  /* ArtNet net/subnet/uni address, must match artnet_options.address */
  uint16_t address;

  /* Task associated with output, will receive task notifications on updates */
  xTaskHandle task;
};

struct artnet_output_state {
  /* Last received ArtDmx seq */
  uint8_t seq;
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

/** Patch multiple output ports.
 *
 * Up to 16 total output ports are supported, indexed across four physical ports.
 * All output port addresses must use an output universe matching the artnet_options.universe subnet, i.e. only the lower 4 bits can vary across ports.
 *
 * With multiple outputs, a direct-to-task notification with a bit matching the output index will be sent for each queue write.
 *
 * @param artnet
 * @param options Art-Net universe address, upper bits must match artnet_options.universe & 0xfff0
 * @param index differentiate between multiple outputs for a task. Also used as ArtPollReply bind index
 * @param queue `struct artnet_dmx` queue of size 1, overwritten from the artnet task
 * @param task send direct-to-task notification for queue writes
 *
 * NOT concurrent-safe, must be called between artnet_new() and artnet_main()!
 */
int artnet_add_output(struct artnet *artnet, struct artnet_output_options options, xQueueHandle queue);

/*
 * Return number of outputs.
 */
unsigned artnet_get_output_count(struct artnet *artnet);

/*
 * Return information about configured artnet outputs.
 *
 * @param artnet
 * @param outputs array of *size artnet_output_options structs
 * @param size input size of array; output number of outputs, may be larger than input
 *
 * Returns <0 on error, 0 on success.
 */
int artnet_get_outputs(struct artnet *artnet, struct artnet_output_options *outputs, size_t *size);

/**
 * Return current state information for output.
 *
 * @param artnet
 * @param index see artnet_get_output_count
 * @param state returned
 *
 * @return <0 on error, 0 on success, >0 on invalid index.
 */
int artnet_get_output_state(struct artnet *artnet, int index, struct artnet_output_state *state);

/**
 * Set all artnet outputs into test mode. The outputs will return into normal operating mode as soon
 * as they receive any real data.
 *
 * This only works for outputs with an associated (struct artnet_output_options *)->task.
 */
int artnet_test_outputs(struct artnet *artnet);

/** Run artnet mainloop.
 *
 * Logs warnings for protocol errors.
 *
 * Returns on network error.
 */
int artnet_main(struct artnet *artnet);

#endif

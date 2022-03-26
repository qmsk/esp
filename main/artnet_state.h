#pragma once

#include <artnet.h>

// custom artnet_output event_group bits
#define LEDS_ARTNET_EVENT_SYNC_BIT 16 // ARTNET_OUTPUT_EVENT_SYNC_BIT
#define LEDS_ARTNET_EVENT_TEST_BIT 17

extern struct artnet *artnet;

/*
 * options.address: universe: 0-15 to be combined with artnet net/subnet
 */
int add_artnet_input(struct artnet_input **inputp, struct artnet_input_options options);

/*
 * options.address: universe: 0-15 to be combined with artnet net/subnet
 */
int add_artnet_output(struct artnet_output **outputp, struct artnet_output_options options);

/*
 * Used to notify LEDS_ARTNET_EVENT_TEST/SYNC_BIT.
 */
void notify_artnet_outputs(EventBits_t bits);

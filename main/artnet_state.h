#pragma once

#include <artnet.h>

extern struct artnet *artnet;

/*
 * options.address: universe: 0-15 to be combined with artnet net/subnet
 */
int add_artnet_input(struct artnet_input **inputp, struct artnet_input_options options);

/*
 * options.address: universe: 0-15 to be combined with artnet net/subnet
 */
int add_artnet_output(struct artnet_output **outputp, struct artnet_output_options options);

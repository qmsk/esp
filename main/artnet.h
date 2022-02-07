#pragma once

#include <cmd.h>
#include <config.h>

extern const struct configtab artnet_configtab[];
extern const struct cmdtab artnet_cmdtab;

int init_artnet();

/*
 * start artnet receiver, once outputs are setup.
 */
int start_artnet();

/*
 * reconfigure artnet receiver
 */
int update_artnet_network();

/*
 * Test artnet outputs
 */
void test_artnet();
void cancel_artnet_test();

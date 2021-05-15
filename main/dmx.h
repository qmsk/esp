#pragma once

#include <cmd.h>
#include <config.h>

extern const struct configtab dmx_configtab[];

int init_dmx();
int init_dmx_artnet(uint16_t universe);

int output_dmx(void *data, size_t len);

extern const struct cmdtab dmx_cmdtab;

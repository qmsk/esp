#ifndef __USER_DMX_H__
#define __USER_DMX_H__

#include "dmx_config.h"

enum dmx_cmd {
    DMX_CMD_DIMMER = 0x00,
};

int init_dmx(struct dmx_config *config);

#endif

#ifndef __USER_DMX_H__
#define __USER_DMX_H__

#include "user_config.h"

enum dmx_cmd {
    DMX_CMD_DIMMER = 0x00,
};

int init_dmx(struct user_config *config);

extern const struct cmdtab dmx_cmdtab;

#endif

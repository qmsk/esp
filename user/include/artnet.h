#ifndef __USER_ARTNET_H__
#define __USER_ARTNET_H__

#include "user_config.h"
#include "user_info.h"

typedef void artnet_output_func(uint8_t *data, size_t len, void *arg);

int init_artnet(const struct user_config *config, const struct user_info *user_info);

// TODO: Use FreeRTOS queue..
int patch_artnet_output(uint16_t addr, artnet_output_func *func, void *arg);

#endif

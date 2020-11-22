#ifndef __USER_ARTNET_H__
#define __USER_ARTNET_H__

#include "artnet_config.h"
#include "user_info.h"

int init_artnet(const struct artnet_config *config, const struct user_info *user_info);

#endif

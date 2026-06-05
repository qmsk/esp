#pragma once

#include <artnet.h>

struct artnet_status {
    bool sync_mode;
};

struct artnet_status get_artnet_status(struct artnet *artnet);

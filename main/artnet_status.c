#include "artnet_status.h"

struct artnet_status get_artnet_status(struct artnet *artnet)
{
    return (struct artnet_status) {
        .sync_mode = artnet_is_sync_state(artnet),
    };
}

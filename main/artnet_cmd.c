#include "artnet.h"
#include "artnet_cmd.h"

#include <artnet.h>
#include <logging.h>

int artnet_cmd_info(int argc, char **argv, void *ctx)
{
  struct artnet_options options = artnet_get_options(artnet);

  printf("Listen port=%u\n", options.port);
  printf("Address net=%u subnet=%u\n", artnet_address_net(options.address), artnet_address_subnet(options.address));
  printf("Network IPv4=%u.%u.%u.%u MAC=%02x:%02x:%02x:%02x:%02x:%02x\n",
    options.ip_address[0], options.ip_address[1], options.ip_address[2], options.ip_address[3],
    options.mac_address[0], options.mac_address[1], options.mac_address[2], options.mac_address[3], options.mac_address[4], options.mac_address[5]
  );
  printf("Name short=%s long=%s\n", options.short_name, options.long_name);

  return 0;
}

const struct cmd artnet_commands[] = {
  { "info",   artnet_cmd_info,    .usage = "",                      .describe = "Show configuration" },
  { }
};

const struct cmdtab artnet_cmdtab = {
  .commands = artnet_commands,
};

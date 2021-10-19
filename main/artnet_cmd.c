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

#define ARTNET_OUTPUT_COUNT 16

int artnet_cmd_outputs(int argc, char **argv, void *ctx)
{
  struct artnet_output_info artnet_output_infos[ARTNET_OUTPUT_COUNT];
  size_t size = ARTNET_OUTPUT_COUNT;
  int err;

  if ((err = artnet_get_outputs(artnet, artnet_output_infos, &size))) {
    LOG_ERROR("artnet_get_outputs");
    return err;
  }

  for (int i = 0; i < size && i < ARTNET_OUTPUT_COUNT; i++) {
    struct artnet_output_info *info = &artnet_output_infos[i];

    printf("Output %2d: net=%3u subnet=%2u universe=%2u (index=%3u)\n", i,
      artnet_address_net(info->address), artnet_address_subnet(info->address), artnet_address_universe(info->address),
      info->index
    );
    printf("\tSeq: %u\n", info->seq);
    printf("\n");
  }

  return 0;
}

const struct cmd artnet_commands[] = {
  { "info",      artnet_cmd_info,     .usage = "",                      .describe = "Show configuration" },
  { "outputs",   artnet_cmd_outputs,  .usage = "",                      .describe = "Show output configuration and status" },
  { }
};

const struct cmdtab artnet_cmdtab = {
  .commands = artnet_commands,
};

#include "eth.h"
#include "eth_state.h"

#include <logging.h>

#if CONFIG_ETH_ENABLED
  #include <system_eth.h>

  #include <esp_eth.h>
  #include <esp_err.h>

  static int eth_info_cmd(int argc, char **argv, void *ctx)
  {
    esp_eth_handle_t eth_handle = get_eth_handle();
    uint8_t mac[6];
    eth_link_t link;
    eth_speed_t speed;
    eth_duplex_t duplex;
    esp_err_t err;

    if ((err = esp_eth_ioctl(eth_handle, ETH_CMD_G_LINK, &link))) {
      LOG_ERROR("esp_eth_ioctl ETH_CMD_G_LINK: %s", esp_err_to_name(err));
    }
    if ((err = esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac))) {
      LOG_ERROR("esp_eth_ioctl ETH_CMD_G_MAC_ADDR: %s", esp_err_to_name(err));
    }
    if ((err = esp_eth_ioctl(eth_handle, ETH_CMD_G_SPEED, &speed))) {
      LOG_ERROR("esp_eth_ioctl ETH_CMD_G_SPEED: %s", esp_err_to_name(err));
    }
    if ((err = esp_eth_ioctl(eth_handle, ETH_CMD_G_DUPLEX_MODE, &duplex))) {
      LOG_ERROR("esp_eth_ioctl ETH_CMD_G_DUPLEX_MODE: %s", esp_err_to_name(err));
    }

    printf("ETH %02x:%02x:%02x:%02x:%02x:%02x:\n",
      mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
    );

    printf("\t%-20s: %.32s\n", "Link", eth_link_str(link));
    printf("\t%-20s: %.32s\n", "Speed", eth_speed_str(speed));
    printf("\t%-20s: %.32s\n", "Duplex", eth_duplex_str(duplex));

    return 0;
  }
#endif

const struct cmd eth_commands[] = {
#if CONFIG_ETH_ENABLED
  { "info",       eth_info_cmd,         .usage = "",              .describe = "Show MAC/PHY status"   },
#endif
  {}
};

const struct cmdtab eth_cmdtab = {
  .commands = eth_commands,
};

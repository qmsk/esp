#include "../eth.h"
#include "../eth_netif.h"
#include "../user.h"

#include <esp_eth.h>
#include <esp_event.h>

#include <logging.h>
#include <system_eth.h>

static void on_eth_start(esp_eth_handle_t eth_handle)
{
  LOG_INFO("");

  user_state(USER_STATE_DISCONNECTED);
}

static void on_eth_stop(esp_eth_handle_t eth_handle)
{
  LOG_INFO("");

  user_state(USER_STATE_STOPPED);
}

static void on_eth_connected(esp_eth_handle_t eth_handle)
{
  uint32_t phy_addr;
  uint8_t mac[6];
  eth_speed_t eth_speed;
  eth_duplex_t eth_duplex;
  esp_err_t err;

  if ((err = esp_eth_ioctl(eth_handle, ETH_CMD_G_PHY_ADDR, &phy_addr))) {
    LOG_ERROR("esp_eth_ioctl ETH_CMD_G_PHY_ADDR: %s", esp_err_to_name(err));
  }
  if ((err = esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac))) {
    LOG_ERROR("esp_eth_ioctl ETH_CMD_G_MAC_ADDR: %s", esp_err_to_name(err));
  }
  if ((err = esp_eth_ioctl(eth_handle, ETH_CMD_G_SPEED, &eth_speed))) {
    LOG_ERROR("esp_eth_ioctl ETH_CMD_G_SPEED: %s", esp_err_to_name(err));
  }
  if ((err = esp_eth_ioctl(eth_handle, ETH_CMD_G_DUPLEX_MODE, &eth_duplex))) {
    LOG_ERROR("esp_eth_ioctl ETH_CMD_G_DUPLEX_MODE: %s", esp_err_to_name(err));
  }

  LOG_INFO("link up: phy=%u mac=%02x:%02x:%02x:%02x:%02x:%02x speed=%s duplex=%s",
    phy_addr,
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5],
    eth_speed_str(eth_speed),
    eth_duplex_str(eth_duplex)
  );

  if (is_eth_netif_dhcpc() > 0) {
    // wait for IP_EVENT_ETH_GOT_IP before USER_STATE_CONNECTED
    user_state(USER_STATE_CONNECTING);
  } else {
    user_state(USER_STATE_CONNECTED);
  }
}

static void on_eth_disconnected(esp_eth_handle_t eth_handle)
{
  uint32_t phy_addr;
  uint8_t mac[6];
  esp_err_t err;

  if ((err = esp_eth_ioctl(eth_handle, ETH_CMD_G_PHY_ADDR, &phy_addr))) {
    LOG_ERROR("esp_eth_ioctl ETH_CMD_G_MAC_ADDR: %s", esp_err_to_name(err));
  }
  if ((err = esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac))) {
    LOG_ERROR("esp_eth_ioctl ETH_CMD_G_MAC_ADDR: %s", esp_err_to_name(err));
  }

  LOG_INFO("link down: phy=%u mac=%02x:%02x:%02x:%02x:%02x:%02x",
    phy_addr,
    mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]
  );

  user_state(USER_STATE_DISCONNECTED);
}

static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  esp_eth_handle_t *handlep = event_data;

  switch ((eth_event_t) event_id) {
    case ETHERNET_EVENT_START:
      return on_eth_start(*handlep);

    case ETHERNET_EVENT_STOP:
      return on_eth_stop(*handlep);

    case ETHERNET_EVENT_CONNECTED:
      return on_eth_connected(*handlep);

    case ETHERNET_EVENT_DISCONNECTED:
      return on_eth_disconnected(*handlep);
  }
}

int init_eth_events()
{
  esp_err_t err;

  if ((err = esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, &eth_event_handler, NULL))) {
    LOG_ERROR("esp_event_handler_register ETH_EVENT: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

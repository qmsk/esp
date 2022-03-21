#include "../eth.h"
#include "../eth_state.h"

#include <esp_eth.h>
#include <esp_err.h>
#include <esp_netif.h>

#include <logging.h>

#include <sdkconfig.h>

// WT32-ETH01
#define ETH_MAC_MDC_GPIO_NUM 23
#define ETH_MAC_MDIO_GPIO_NUM 18
#define ETH_PHY_LAN87XX 1
#define ETH_PHY_ADDR 1
#define ETH_PHY_RESET_GPIO_NUM 16 // not actually wired to LAN87XX nRST pin, but the 50MHz OSC EN pin - close enough, both are active-high

static esp_eth_mac_t *eth_mac;
static esp_eth_phy_t *eth_phy;
static esp_eth_handle_t eth_handle;
static esp_netif_t *eth_netif;
static esp_eth_netif_glue_handle_t eth_netif_glue;

esp_eth_handle_t get_eth_handle()
{
  return eth_handle;
}

static int init_eth_mac()
{
  eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();

  mac_config.smi_mdc_gpio_num = CONFIG_ETH_MAC_MDC_GPIO_NUM;
  mac_config.smi_mdio_gpio_num = CONFIG_ETH_MAC_MDIO_GPIO_NUM;
  mac_config.flags |= ETH_MAC_FLAG_PIN_TO_CORE;

  LOG_INFO("smi_mdc_gpio_num=%d smi_mdio_gpio_num=%d",
    mac_config.smi_mdc_gpio_num,
    mac_config.smi_mdio_gpio_num
  );

  if (!(eth_mac = esp_eth_mac_new_esp32(&mac_config))) {
    LOG_ERROR("esp_eth_mac_new_esp32");
    return -1;
  }

  return 0;
}

static int init_eth_phy()
{
  eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

  phy_config.phy_addr = CONFIG_ETH_PHY_ADDR;
  phy_config.reset_gpio_num = CONFIG_ETH_PHY_RESET_GPIO_NUM;

#if CONFIG_ETH_PHY_LAN87XX
  LOG_INFO("lan87xx: phy_addr=%d reset_gpio_num=%d",
    phy_config.phy_addr,
    phy_config.reset_gpio_num
  );

  if (!(eth_phy = esp_eth_phy_new_lan87xx(&phy_config))) {
    LOG_ERROR("esp_eth_phy_new_lan87xx");
    return -1;
  }
#else
# error No CONFIG_ETH_PHY_* configured
#endif

  return 0;
}

static int init_eth_driver()
{
  esp_eth_config_t eth_config = ETH_DEFAULT_CONFIG(eth_mac, eth_phy);
  esp_err_t err;

  if ((err = esp_eth_driver_install(&eth_config, &eth_handle))) {
    LOG_ERROR("esp_eth_driver_install: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

static int init_eth_netif()
{
  esp_netif_config_t netif_config = ESP_NETIF_DEFAULT_ETH();
  esp_err_t err;

  if (!(eth_netif = esp_netif_new(&netif_config))) {
    LOG_ERROR("esp_netif_new");
    return -1;
  }

  if (!(eth_netif_glue = esp_eth_new_netif_glue(eth_handle))) {
    LOG_ERROR("esp_eth_new_netif_glue");
    return -1;
  }

  if ((err = esp_netif_attach(eth_netif, eth_netif_glue))) {
    LOG_ERROR("esp_netif_attach: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

int init_eth()
{
  int err;

  if ((err = init_eth_mac())) {
    LOG_ERROR("init_eth_mac");
    return err;
  }

  if ((err = init_eth_phy())) {
    LOG_ERROR("init_eth_phy");
    return err;
  }

  if ((err = init_eth_driver())) {
    LOG_ERROR("init_eth_driver");
    return err;
  }

  if ((err = init_eth_netif())) {
    LOG_ERROR("init_eth_netif");
    return err;
  }

  if ((err = init_eth_events())) {
    LOG_ERROR("init_eth_events");
    return err;
  }

  return 0;
}

int start_eth()
{
  esp_err_t err;

  if (!eth_handle) {
    LOG_INFO("not initialized");
    return 0;
  }

  LOG_INFO("");

  if ((err = esp_eth_start(eth_handle))) {
    LOG_ERROR("esp_eth_start: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

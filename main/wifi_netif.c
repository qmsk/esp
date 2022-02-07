#include "wifi.h"
#include "wifi_state.h"

#include <esp_err.h>
#include <esp_netif.h>
#include <esp_wifi.h>

#include <logging.h>
#include <system_wifi.h>

esp_netif_t *wifi_netif[] = {
  [WIFI_IF_STA] = NULL,
  [WIFI_IF_AP]  = NULL,
};

static int create_wifi_netif(esp_netif_t **netifp, wifi_interface_t interface)
{
  switch(interface) {
    case WIFI_IF_STA:
      if (!(*netifp = esp_netif_create_default_wifi_sta())) {
        LOG_ERROR("esp_netif_create_default_wifi_sta");
        return -1;
      }

      return 0;

    case WIFI_IF_AP:
      if (!(*netifp = esp_netif_create_default_wifi_ap())) {
        LOG_ERROR("esp_netif_create_default_wifi_ap");
        return -1;
      }

      return 0;

    default:
      LOG_ERROR("invalid interface=%d", interface);
      return -1;
  }
}

int make_wifi_netif(esp_netif_t **netifp, wifi_interface_t interface)
{
  esp_netif_t *netif;
  int err;

  if ((netif = wifi_netif[interface])) {

  } else if ((err = create_wifi_netif(&netif, interface))) {
    LOG_ERROR("create_wifi_netif(%d)", interface);
    return err;
  } else {
    LOG_INFO("created wifi_netif[%s]: %s", wifi_interface_str(interface), esp_netif_get_desc(netif));

    wifi_netif[interface] = netif;
  }

  if (netifp) {
    *netifp = netif;
  }

  return 0;
}

int init_wifi_netif(wifi_interface_t interface)
{
  return make_wifi_netif(NULL, interface);
}

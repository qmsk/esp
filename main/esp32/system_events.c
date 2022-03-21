#include "system_netif.h"
#include "../system_init.h"
#include "../user.h"

#include <logging.h>

#include <esp_event.h>
#include <esp_netif_types.h>

static void on_ip_sta_got_ip(ip_event_got_ip_t *event)
{
  LOG_INFO("if=%s ip=%d.%d.%d.%d changed=%s",
    esp_netif_get_desc(event->esp_netif),
    esp_ip4_addr1(&event->ip_info.ip), esp_ip4_addr2(&event->ip_info.ip), esp_ip4_addr3(&event->ip_info.ip), esp_ip4_addr4(&event->ip_info.ip),
    event->ip_changed ? "true" : "false"
  );

  user_state(USER_STATE_CONNECTED);

  system_netif_connected(event->esp_netif);
}

static void on_ip_sta_lost_ip()
{
  LOG_INFO(" ");

  // assume only one and the same interface
  system_netif_disconnected();
}

static void on_ip_ap_sta_ip_assigned(ip_event_ap_staipassigned_t *event)
{
  LOG_INFO("ip=%d.%d.%d.%d",
    esp_ip4_addr1(&event->ip), esp_ip4_addr2(&event->ip), esp_ip4_addr3(&event->ip), esp_ip4_addr4(&event->ip)
  );
}

static void on_ip_got_ip6(ip_event_got_ip6_t *event)
{
  LOG_INFO("if=%s ip=%4x:%4x:%4x:%4x:%4x:%4x:%4x:%4x",
    esp_netif_get_desc(event->esp_netif),
    ESP_IP6_ADDR_BLOCK1(&event->ip6_info.ip),
    ESP_IP6_ADDR_BLOCK2(&event->ip6_info.ip),
    ESP_IP6_ADDR_BLOCK4(&event->ip6_info.ip),
    ESP_IP6_ADDR_BLOCK3(&event->ip6_info.ip),
    ESP_IP6_ADDR_BLOCK5(&event->ip6_info.ip),
    ESP_IP6_ADDR_BLOCK6(&event->ip6_info.ip),
    ESP_IP6_ADDR_BLOCK7(&event->ip6_info.ip),
    ESP_IP6_ADDR_BLOCK8(&event->ip6_info.ip)
  );
}

static void on_ip_eth_got_ip(ip_event_got_ip_t *event)
{
  LOG_INFO("if=%s ip=%d.%d.%d.%d changed=%s",
    esp_netif_get_desc(event->esp_netif),
    esp_ip4_addr1(&event->ip_info.ip), esp_ip4_addr2(&event->ip_info.ip), esp_ip4_addr3(&event->ip_info.ip), esp_ip4_addr4(&event->ip_info.ip),
    event->ip_changed ? "true" : "false"
  );

  user_state(USER_STATE_CONNECTED);

  system_netif_connected(event->esp_netif);
}

static void on_ip_eth_lost_ip()
{
  LOG_INFO(" ");

  // assume only one and the same interface
  system_netif_disconnected();
}

static void ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
  switch (event_id) {
    case IP_EVENT_STA_GOT_IP:
      return on_ip_sta_got_ip(event_data);

    case IP_EVENT_STA_LOST_IP:
      return on_ip_sta_lost_ip();

    case IP_EVENT_AP_STAIPASSIGNED:
      return on_ip_ap_sta_ip_assigned(event_data);

    case IP_EVENT_GOT_IP6:
      return on_ip_got_ip6(event_data);

    case IP_EVENT_ETH_GOT_IP:
      return on_ip_eth_got_ip(event_data);

    case IP_EVENT_ETH_LOST_IP:
      return on_ip_eth_lost_ip();
  }
}

int init_system_events()
{
  esp_err_t err;

  if ((err = esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &ip_event_handler, NULL))) {
    LOG_ERROR("esp_event_handler_register IP_EVENT: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}

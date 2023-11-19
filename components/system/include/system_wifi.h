#pragma once

#include <esp_wifi.h>

const char *wifi_interface_str(wifi_interface_t iface);
const char *wifi_mode_str(wifi_mode_t mode);
const char *wifi_auth_mode_str(wifi_auth_mode_t auth_mode);
const char *wifi_cipher_type_str(wifi_cipher_type_t cipher_type);
const char *wifi_err_reason_str(wifi_err_reason_t reason);

#if !CONFIG_IDF_TARGET_ESP8266
  const char *wifi_phy_mode_str(wifi_phy_mode_t phymode);
#endif

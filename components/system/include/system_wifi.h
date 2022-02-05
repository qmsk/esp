#pragma once

#include <sdkconfig.h>

// TODO
#if CONFIG_IDF_TARGET_ESP8266

#include <esp_wifi.h>

const char *wifi_interface_str(wifi_interface_t iface);
const char *wifi_mode_str(wifi_mode_t mode);
const char *wifi_auth_mode_str(wifi_auth_mode_t auth_mode);
const char *wifi_cipher_type_str(wifi_cipher_type_t cipher_type);

#endif

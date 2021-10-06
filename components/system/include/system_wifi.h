#ifndef __SYSTEM_WIFI_H__
#define __SYSTEM_WIFI_H__

#include <esp_wifi.h>

const char *wifi_mode_str(wifi_mode_t mode);
const char *wifi_auth_mode_str(wifi_auth_mode_t auth_mode);
const char *wifi_cipher_type_str(wifi_cipher_type_t cipher_type);

#endif

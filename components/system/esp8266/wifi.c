#include <system_wifi.h>

const char *wifi_interface_str(wifi_interface_t iface)
{
  switch(iface) {
    case WIFI_IF_STA:     return "STA";
    case WIFI_IF_AP:      return "AP";
    default:              return "?";
  }
}
const char *wifi_mode_str(wifi_mode_t mode)
{
  switch(mode) {
    case WIFI_MODE_NULL:      return "NULL";
    case WIFI_MODE_STA:       return "STA";
    case WIFI_MODE_AP:        return "AP";
    case WIFI_MODE_APSTA:     return "APSTA";
    default:                  return "?";
  }
}

const char *wifi_auth_mode_str(wifi_auth_mode_t auth_mode) {
  switch (auth_mode) {
    case WIFI_AUTH_OPEN:            return "OPEN";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/2-PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3-PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/3-PSK";
    default:                        return "?";
  }
}

const char *wifi_cipher_type_str(wifi_cipher_type_t cipher_type) {
  switch (cipher_type) {
    case WIFI_CIPHER_TYPE_NONE:         return "NONE";
    case WIFI_CIPHER_TYPE_WEP40:        return "WEP40";
    case WIFI_CIPHER_TYPE_WEP104:       return "WEP104";
    case WIFI_CIPHER_TYPE_TKIP:         return "TKIP";
    case WIFI_CIPHER_TYPE_CCMP:         return "CCMP";
    case WIFI_CIPHER_TYPE_TKIP_CCMP:    return "TKIP-CCMP";
    case WIFI_CIPHER_TYPE_AES_CMAC128:  return "AES-CMAC128";
    case WIFI_CIPHER_TYPE_UNKNOWN:      return "UNKNOWN";
    default:                            return "?";
  }
}

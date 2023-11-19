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
#if CONFIG_IDF_TARGET_ESP32
    case WIFI_AUTH_WAPI_PSK:        return "WPAI-PSK";
#endif
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
#if CONFIG_IDF_TARGET_ESP32
    case WIFI_CIPHER_TYPE_SMS4:         return "SMS4";
    case WIFI_CIPHER_TYPE_GCMP:         return "GCMP";
    case WIFI_CIPHER_TYPE_GCMP256:      return "GCMP256";
    case WIFI_CIPHER_TYPE_AES_GMAC128:  return "AES_GMAC128";
    case WIFI_CIPHER_TYPE_AES_GMAC256:  return "AES_GMAC256";
#endif
    case WIFI_CIPHER_TYPE_UNKNOWN:      return "UNKNOWN";
    default:                            return "?";
  }
}

const char *wifi_err_reason_str(wifi_err_reason_t reason) {
  switch (reason) {
    case WIFI_REASON_UNSPECIFIED:               return "UNSPECIFIED";
    case WIFI_REASON_AUTH_EXPIRE:               return "AUTH_EXPIRE";
    case WIFI_REASON_AUTH_LEAVE:                return "AUTH_LEAVE";
    case WIFI_REASON_ASSOC_EXPIRE:              return "ASSOC_EXPIRE";
    case WIFI_REASON_ASSOC_TOOMANY:             return "ASSOC_TOOMANY";
    case WIFI_REASON_NOT_AUTHED:                return "NOT_AUTHED";
    case WIFI_REASON_NOT_ASSOCED:               return "NOT_ASSOCED";
    case WIFI_REASON_ASSOC_LEAVE:               return "ASSOC_LEAVE";
    case WIFI_REASON_ASSOC_NOT_AUTHED:          return "ASSOC_NOT_AUTHED";
    case WIFI_REASON_DISASSOC_PWRCAP_BAD:       return "DISASSOC_PWRCAP_BAD";
    case WIFI_REASON_DISASSOC_SUPCHAN_BAD:      return "DISASSOC_SUPCHAN_BAD";
    case WIFI_REASON_IE_INVALID:                return "IE_INVALID";
    case WIFI_REASON_MIC_FAILURE:               return "MIC_FAILURE";
    case WIFI_REASON_4WAY_HANDSHAKE_TIMEOUT:    return "4WAY_HANDSHAKE_TIMEOUT";
    case WIFI_REASON_GROUP_KEY_UPDATE_TIMEOUT:  return "GROUP_KEY_UPDATE_TIMEOUT";
    case WIFI_REASON_IE_IN_4WAY_DIFFERS:        return "IE_IN_4WAY_DIFFERS";
    case WIFI_REASON_GROUP_CIPHER_INVALID:      return "GROUP_CIPHER_INVALID";
    case WIFI_REASON_PAIRWISE_CIPHER_INVALID:   return "PAIRWISE_CIPHER_INVALID";
    case WIFI_REASON_AKMP_INVALID:              return "AKMP_INVALID";
    case WIFI_REASON_UNSUPP_RSN_IE_VERSION:     return "UNSUPP_RSN_IE_VERSION";
    case WIFI_REASON_INVALID_RSN_IE_CAP:        return "INVALID_RSN_IE_CAP";
    case WIFI_REASON_802_1X_AUTH_FAILED:        return "802_1X_AUTH_FAILED";
    case WIFI_REASON_CIPHER_SUITE_REJECTED:     return "CIPHER_SUITE_REJECTED";

    case WIFI_REASON_INVALID_PMKID:             return "INVALID_PMKID";

    case WIFI_REASON_BEACON_TIMEOUT:            return "BEACON_TIMEOUT";
    case WIFI_REASON_NO_AP_FOUND:               return "NO_AP_FOUND";
    case WIFI_REASON_AUTH_FAIL:                 return "AUTH_FAIL";
    case WIFI_REASON_ASSOC_FAIL:                return "ASSOC_FAIL";
    case WIFI_REASON_HANDSHAKE_TIMEOUT:         return "HANDSHAKE_TIMEOUT";
    case WIFI_REASON_CONNECTION_FAIL:           return "CONNECTION_FAIL";
    case WIFI_REASON_AP_TSF_RESET:              return "AP_TSF_RESET";
#if CONFIG_IDF_TARGET_ESP8266
    case WIFI_REASON_BASIC_RATE_NOT_SUPPORT:    return "BASIC_RATE_NOT_SUPPORT";
#elif CONFIG_IDF_TARGET_ESP32
    case WIFI_REASON_ROAMING:                   return "ROAMING";
#endif

    default: return "?";
  }
}

#if !CONFIG_IDF_TARGET_ESP8266
  const char *wifi_phy_mode_str(wifi_phy_mode_t phymode)
  {
    switch (phymode) {
      case WIFI_PHY_MODE_LR:    return "LR";
      case WIFI_PHY_MODE_11B:   return "11B";
      case WIFI_PHY_MODE_11G:   return "11G";
      case WIFI_PHY_MODE_HT20:  return "HT20";
      case WIFI_PHY_MODE_HT40:  return "HT40";
      case WIFI_PHY_MODE_HE20:  return "HE20";
      default:                  return "?";
    }
  }
#endif

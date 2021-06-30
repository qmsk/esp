#include <esp_partition.h>

const char *esp_partition_type_str(esp_partition_type_t type)
{
  switch (type) {
    case ESP_PARTITION_TYPE_APP: return "APP";
    case ESP_PARTITION_TYPE_DATA: return "DATA";
    default: return NULL;
  }
}

const char *esp_partition_subtype_str(esp_partition_type_t type, esp_partition_subtype_t subtype)
{
  switch (type) {
    case ESP_PARTITION_TYPE_APP:
      switch(subtype) {
        case ESP_PARTITION_SUBTYPE_APP_FACTORY: return "FACTORY";
        case ESP_PARTITION_SUBTYPE_APP_OTA_0: return "OTA_0";
        case ESP_PARTITION_SUBTYPE_APP_OTA_1: return "OTA_1";
        case ESP_PARTITION_SUBTYPE_APP_OTA_2: return "OTA_2";
        case ESP_PARTITION_SUBTYPE_APP_OTA_3: return "OTA_3";
        case ESP_PARTITION_SUBTYPE_APP_OTA_4: return "OTA_4";
        case ESP_PARTITION_SUBTYPE_APP_OTA_5: return "OTA_5";
        case ESP_PARTITION_SUBTYPE_APP_OTA_6: return "OTA_6";
        case ESP_PARTITION_SUBTYPE_APP_OTA_7: return "OTA_7";
        case ESP_PARTITION_SUBTYPE_APP_OTA_8: return "OTA_8";
        case ESP_PARTITION_SUBTYPE_APP_OTA_9: return "OTA_9";
        case ESP_PARTITION_SUBTYPE_APP_OTA_10: return "OTA_10";
        case ESP_PARTITION_SUBTYPE_APP_OTA_11: return "OTA_11";
        case ESP_PARTITION_SUBTYPE_APP_OTA_12: return "OTA_12";
        case ESP_PARTITION_SUBTYPE_APP_OTA_13: return "OTA_13";
        case ESP_PARTITION_SUBTYPE_APP_OTA_14: return "OTA_14";
        case ESP_PARTITION_SUBTYPE_APP_OTA_15: return "OTA_15";
        case ESP_PARTITION_SUBTYPE_APP_TEST: return "TEST";
        default: return NULL;
      }

    case ESP_PARTITION_TYPE_DATA:
      switch(subtype) {
        case ESP_PARTITION_SUBTYPE_DATA_OTA: return "OTA";
        case ESP_PARTITION_SUBTYPE_DATA_PHY: return "PHY";
        case ESP_PARTITION_SUBTYPE_DATA_NVS: return "NVS";
        case ESP_PARTITION_SUBTYPE_DATA_COREDUMP: return "COREDUMP";
        case ESP_PARTITION_SUBTYPE_DATA_ESPHTTPD: return "ESPHTTPD";
        case ESP_PARTITION_SUBTYPE_DATA_FAT: return "FAT";
        case ESP_PARTITION_SUBTYPE_DATA_SPIFFS: return "SPIFFS";
        default: return NULL;
      }

    default: return NULL;
  }
}

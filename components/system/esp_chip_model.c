#include <system.h>

#include <esp_system.h>
#include <sdkconfig.h>

const char *esp_chip_model_str(esp_chip_model_t model)
{
  switch (model) {
#if CONFIG_IDF_TARGET_ESP8266
    case CHIP_ESP8266:  return "ESP8266";
#endif
    case CHIP_ESP32:    return "ESP32";
    default:            return "?";
  }
}

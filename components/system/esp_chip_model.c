#include <system.h>

const char *esp_chip_model_str(esp_chip_model_t model)
{
  switch (model) {
    case CHIP_ESP8266: return "ESP8266";
    case CHIP_ESP32: return "ESP32";
    default: return "?";
  }
}

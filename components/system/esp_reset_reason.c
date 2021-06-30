#include <system.h>

const char *esp_reset_reason_str(esp_reset_reason_t reason)
{
  switch (reason) {
    case ESP_RST_UNKNOWN: return "Reset reason can not be determined";
    case ESP_RST_POWERON: return "Reset due to power-on event";
    case ESP_RST_EXT: return "Reset by external pin (not applicable for ESP8266)";
    case ESP_RST_SW: return "Software reset via esp_restart";
    case ESP_RST_PANIC: return "Software reset due to exception/panic";
    case ESP_RST_INT_WDT: return "Reset (software or hardware) due to interrupt watchdog";
    case ESP_RST_TASK_WDT: return "Reset due to task watchdog";
    case ESP_RST_WDT: return "Reset due to other watchdogs";
    case ESP_RST_DEEPSLEEP: return "Reset after exiting deep sleep mode";
    case ESP_RST_BROWNOUT: return "Brownout reset (software or hardware)";
    case ESP_RST_SDIO: return "Reset over SDIO";
    case ESP_RST_FAST_SW: return "Fast reboot";
    default: return "Unknown";
  }
}

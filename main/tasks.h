#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// used for LEDs ArtNET output -> leds interfaces
#define LEDS_ARTNET_TASK_NAME_FMT "leds%d"
#define LEDS_ARTNET_TASK_PRIORITY (tskIDLE_PRIORITY + 10)

#if CONFIG_IDF_TARGET_ESP8266
# define LEDS_ARTNET_TASK_STACK 1024
#elif CONFIG_IDF_TARGET_ESP32
# define LEDS_ARTNET_TASK_STACK 2048
#endif

// used for DMX ArtNET output -> uart
#define DMX_OUTPUT_TASK_NAME_FMT "dmx-output%d"
#define DMX_OUTPUT_TASK_PRIORITY (tskIDLE_PRIORITY + 10)

#if CONFIG_IDF_TARGET_ESP8266
# define DMX_OUTPUT_TASK_STACK 1024
#elif CONFIG_IDF_TARGET_ESP32
# define DMX_OUTPUT_TASK_STACK 2048
#endif

// used for UART setup + DMX input
#define DMX_TASK_NAME "dmx"
#define DMX_TASK_PRIORITY (tskIDLE_PRIORITY + 8)
#define DMX_TASK_STACK 2048

// used for ArtNET network protocol, input -> output
#define ARTNET_LISTEN_TASK_NAME "artnet-listen"
#define ARTNET_INPUTS_TASK_NAME "artnet-inputs"
#define ARTNET_TASK_STACK 2048
#define ARTNET_TASK_PRIORITY (tskIDLE_PRIORITY + 6)

// used for polling status led button inputs
#define STATUS_LEDS_TASK_NAME "status-leds"
#define STATUS_LEDS_TASK_PRIORITY (tskIDLE_PRIORITY + 5)

#if CONFIG_IDF_TARGET_ESP8266
# define STATUS_LEDS_TASK_STACK 1024
#elif CONFIG_IDF_TARGET_ESP32
# define STATUS_LEDS_TASK_STACK 2048
#endif

// used for updating status leds
#define STATUS_LEDS_USER_TASK_NAME  "status-led-user"  // max 16 chars
#define STATUS_LEDS_FLASH_TASK_NAME "status-led-flash"
#define STATUS_LEDS_ALERT_TASK_NAME "status-led-alert"
#define STATUS_LED_TASK_PRIORITY (tskIDLE_PRIORITY + 5)

#if CONFIG_IDF_TARGET_ESP8266
# define STATUS_LED_TASK_STACK 512
#elif CONFIG_IDF_TARGET_ESP32
# define STATUS_LED_TASK_STACK 1024
#endif

// uart configuration and management
#define CONSOLE_CLI_TASK_NAME "console-cli"
#define CONSOLE_CLI_TASK_PRIORITY (tskIDLE_PRIORITY + 4)
#if CONFIG_IDF_TARGET_ESP8266
# define CONSOLE_CLI_TASK_STACK 2048 // bytes
#elif CONFIG_IDF_TARGET_ESP32
# define CONSOLE_CLI_TASK_STACK 4096 // bytes
#endif

// network configuration and management, socket IO and API handlers
#define HTTP_SERVER_TASK_NAME "http-server"
#define HTTP_SERVER_TASK_PRIORITY (tskIDLE_PRIORITY + 3)

// network configuration and management, network listen()
#define HTTP_LISTEN_TASK_NAME "http-listen"
#define HTTP_LISTEN_TASK_PRIORITY (tskIDLE_PRIORITY + 2)

#if CONFIG_IDF_TARGET_ESP8266
# define HTTP_LISTEN_TASK_STACK 1024
# define HTTP_SERVER_TASK_STACK 2048
#elif CONFIG_IDF_TARGET_ESP32
# define HTTP_LISTEN_TASK_STACK 2048
# define HTTP_SERVER_TASK_STACK 4096
#endif

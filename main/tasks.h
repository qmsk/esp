#pragma once

#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

struct task_options {
  TaskFunction_t  main;
  char            name[configMAX_TASK_NAME_LEN];
  const char     *name_fmt;
  uint32_t        stack_size;
  void           *arg;
  UBaseType_t     priority;
  TaskHandle_t   *handle;
  BaseType_t      affinity;
};

/* Wrapper for platform-specific xTaskCreate/xTaskCreatePinnedToCore */
int start_task(struct task_options options);
int start_taskf(struct task_options options, ...);

#if CONFIG_IDF_TARGET_ESP8266
# define TASKS_CPU_PRO -1
# define TASKS_CPU_APP -1

#elif CONFIG_IDF_TARGET_ESP32
# define TASKS_CPU_PRO 0 // used for networking and management
# define TASKS_CPU_APP 1 // used for performance-critical tasks

#endif

// used for leds interfaces
#define LEDS_TASK_NAME_FMT "leds%d"
#define LEDS_TASK_PRIORITY (tskIDLE_PRIORITY + 20)
#define LEDS_TASK_AFFINITY TASKS_CPU_APP

#if CONFIG_IDF_TARGET_ESP8266
# define LEDS_TASK_STACK 2048
#elif CONFIG_IDF_TARGET_ESP32
# define LEDS_TASK_STACK 2048
#endif

// used for DMX ArtNET output -> uart
#define DMX_OUTPUT_TASK_NAME_FMT "dmx-output%d"
#define DMX_OUTPUT_TASK_PRIORITY (tskIDLE_PRIORITY + 20)
#define DMX_OUTPUT_TASK_AFFINITY TASKS_CPU_APP

#if CONFIG_IDF_TARGET_ESP8266
# define DMX_OUTPUT_TASK_STACK 2048
#elif CONFIG_IDF_TARGET_ESP32
# define DMX_OUTPUT_TASK_STACK 2048
#endif

// used for UART setup + DMX input
#define DMX_TASK_NAME "dmx"
#define DMX_TASK_PRIORITY (tskIDLE_PRIORITY + 18)
#define DMX_TASK_AFFINITY TASKS_CPU_APP

#define DMX_TASK_STACK 2048

// used for physical UART -> Art-NET inputs
#define ARTNET_INPUTS_TASK_NAME "artnet-inputs"
#define ARTNET_INPUTS_TASK_PRIORITY (tskIDLE_PRIORITY + 10)
#define ARTNET_INPUTS_TASK_AFFINITY TASKS_CPU_APP

#define ARTNET_INPUTS_TASK_STACK 2048

// used for controlling ATX PSU power-on/standby
#define ATX_PSU_TASK_NAME "atx-psu"
#define ATX_PSU_TASK_PRIORITY (tskIDLE_PRIORITY + 6)
#define ATX_PSU_TASK_AFFINITY TASKS_CPU_APP

#if CONFIG_IDF_TARGET_ESP8266
# define ATX_PSU_TASK_STACK 1024
#elif CONFIG_IDF_TARGET_ESP32
# define ATX_PSU_TASK_STACK 2048
#endif

// used for SD-Card fseq -> LEDS
#define LEDS_SEQUENCE_TASK_NAME "leds-sequence"
#define LEDS_SEQUENCE_TASK_PRIORITY (tskIDLE_PRIORITY + 10)
#define LEDS_SEQUENCE_TASK_AFFINITY TASKS_CPU_PRO

#define LEDS_SEQUENCE_TASK_STACK 2048

// used for TCP/IP ArtNET network protocol -> output
#define ARTNET_LISTEN_TASK_NAME "artnet-listen"
#define ARTNET_LISTEN_TASK_PRIORITY (tskIDLE_PRIORITY + 10)
#define ARTNET_LISTEN_TASK_AFFINITY TASKS_CPU_PRO

#define ARTNET_LISTEN_TASK_STACK 2048

// used for handling user events
#define USER_EVENTS_TASK_NAME  "user-events"  // max 16 chars
#define USER_EVENTS_TASK_PRIORITY (tskIDLE_PRIORITY + 6)
#define USER_EVENTS_TASK_AFFINITY TASKS_CPU_PRO

#if CONFIG_IDF_TARGET_ESP8266
# define USER_EVENTS_TASK_STACK 1024
#elif CONFIG_IDF_TARGET_ESP32
# define USER_EVENTS_TASK_STACK 2048
#endif

// used for updating status leds and polling input buttons
#define USER_LEDS_TASK_NAME "user-leds"
#define USER_LEDS_TASK_PRIORITY (tskIDLE_PRIORITY + 5)
#define USER_LEDS_TASK_AFFINITY TASKS_CPU_PRO

#if CONFIG_IDF_TARGET_ESP8266
# define USER_LEDS_TASK_STACK 1024
#elif CONFIG_IDF_TARGET_ESP32
# define USER_LEDS_TASK_STACK 2048
#endif

// used for sdcard hotplug
#define SDCARD_TASK_NAME "sdcard"
#define SDCARD_TASK_PRIORITY (tskIDLE_PRIORITY + 5)
#define SDCARD_TASK_AFFINITY TASKS_CPU_PRO

#if CONFIG_IDF_TARGET_ESP8266
# define SDCARD_TASK_STACK 1024
#elif CONFIG_IDF_TARGET_ESP32
# define SDCARD_TASK_STACK 2048
#endif

// uart configuration and management
#define CONSOLE_CLI_TASK_NAME "console-cli"
#define CONSOLE_CLI_TASK_PRIORITY (tskIDLE_PRIORITY + 4)
#define CONSOLE_CLI_TASK_AFFINITY TASKS_CPU_PRO

#if CONFIG_IDF_TARGET_ESP8266
# define CONSOLE_CLI_TASK_STACK 2048 // bytes
#elif CONFIG_IDF_TARGET_ESP32
# define CONSOLE_CLI_TASK_STACK 4096 // bytes
#endif

// network configuration and management, socket IO and API handlers
#define HTTP_SERVER_TASK_NAME "http-server"
#define HTTP_SERVER_TASK_PRIORITY (tskIDLE_PRIORITY + 3)
#define HTTP_SERVER_TASK_AFFINITY TASKS_CPU_PRO

// network configuration and management, network listen()
#define HTTP_LISTEN_TASK_NAME "http-listen"
#define HTTP_LISTEN_TASK_PRIORITY (tskIDLE_PRIORITY + 2)
#define HTTP_LISTEN_TASK_AFFINITY TASKS_CPU_PRO

#if CONFIG_IDF_TARGET_ESP8266
# define HTTP_LISTEN_TASK_STACK 1024
# define HTTP_SERVER_TASK_STACK 2048
#elif CONFIG_IDF_TARGET_ESP32
# define HTTP_LISTEN_TASK_STACK 2048
# define HTTP_SERVER_TASK_STACK 4096
#endif

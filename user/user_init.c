#include "apa102.h"
#include "artnet.h"
#include "cli.h"
#include "config.h"
#include "dmx.h"
#include "http.h"
#include "led.h"
#include "p9813.h"
#include "spi.h"
#include "spiffs.h"
#include "uart.h"
#include "wifi.h"

#include <lib/logging.h>

#include <esp_common.h>

void print_system()
{
  struct rst_info *rst_info = system_get_rst_info();

  printf("!\tSDK version %s\n", system_get_sdk_version());
  printf("|\tESP8266	chip ID=0x%x\n",	system_get_chip_id());
  printf("|\tReset reason=%d (exc cause=%u)\n", rst_info->reason, rst_info->exccause);
  printf("|\tReset exc: cause=%u epc1=%08x epc2=%08x epc3=%08x excv=%08x depc=%08x rtn=%08x\n",
    rst_info->exccause,
    rst_info->epc1, rst_info->epc2, rst_info->epc3,
    rst_info->excvaddr,
    rst_info->depc,
    rst_info->rtn_addr
  );
  printf("|\tBoot version=%d mode=%d addr=%d\n", system_get_boot_version(), system_get_boot_mode(), system_get_userbin_addr());
  printf("|\tFlash id=%d size_map=%d\n", spi_flash_get_id(), system_get_flash_size_map());
  printf("|\tCPU freq=%d\n", system_get_cpu_freq());
  printf("|\tHeap free=%d\n", system_get_free_heap_size());

  printf("|\tMemory info:\n");
  system_print_meminfo();
  printf("*---------------------------------------\n");
}

void user_event(const struct user_info *info, enum user_event event)
{
  switch (event) {
  case WIFI_CONNECTING:
    LOG_INFO("wifi connecting");
    led_set(LED_FAST);
    break;

  case WIFI_CONNECTED:
    LOG_INFO("wifi connected");
    led_set(LED_SLOW);
    break;

  case WIFI_DISCONNECTED:
    LOG_INFO("wifi disconnected");
    led_set(LED_OFF);
    break;

  default:
    LOG_INFO("unknown (%d)", event);
    led_set(LED_ON);
    break;
  }
}

struct user_info user_info;

void user_init(void)
{
  print_system();

   // status LED high during initial user boot
  if (init_led(LED_ON)) {
    printf("FATAL: init_led\n");
    return;
  }

  if (init_uart()) {
    printf("FATAL: init_uart\n");
    return;
  }

  if (logging_init()) {
    printf("FATAL: init_logging\n");
    return;
  }

  if (init_spiffs()) {
    printf("FATAL: init_spiffs\n");
    return;
  }

  if (init_spi()) {
    printf("FATAL: init_spi\n");
    return;
  }

  if (init_config()) {
    printf("FATAL: init_config\n");
    return;
  }

  if (init_cli()) {
    printf("FATAL: init_cli\n");
    return;
  }

  if (init_wifi(&wifi_config, &user_info, &user_event)) {
    printf("FATAL: init_wifi\n");
    return;
  }

  if (init_http(&http_config) < 0) {
    printf("FATAL: init_http\n");
    return;
  }

  if (init_dmx(&dmx_config)) {
    printf("FATAL: init_dmx\n");
    return;
  }

  if (init_artnet(&artnet_config, &user_info)) {
    printf("FATAL: init_artnet\n");
    return;
  }

  if (init_p9813(&p9813_config)) {
    printf("FATAL: init_p9813\n");
    return;
  }

  if (init_apa102(&apa102_config)) {
    printf("FATAL: init_apa102\n");
    return;
  }

  LOG_INFO("heap free=%u", system_get_free_heap_size());
}

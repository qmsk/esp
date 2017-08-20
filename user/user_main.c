/*
 * ESPRSSIF MIT License
 *
 * Copyright (c) 2015 <ESPRESSIF SYSTEMS (SHANGHAI) PTE LTD>
 *
 * Permission is hereby granted for use on ESPRESSIF SYSTEMS ESP8266 only, in which case,
 * it is free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

#include "user_config.h"
#include "uart.h"
#include "logging.h"
#include "config.h"
#include "cli.h"
#include "spiffs.h"
#include "wifi.h"

#include <esp_common.h>

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 user_rf_cal_sector_set(void)
{
    flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;
        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

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

static struct user_config user_config = {
  .version        = USER_CONFIG_VERSION,
  .wifi_ssid      = USER_CONFIG_WIFI_SSID,
  .wifi_password  = USER_CONFIG_WIFI_PASSWORD,
};

/******************************************************************************
 * FunctionName : user_init
 * Description  : entry of user application, init user function here
 * Parameters   : none
 * Returns      : none
*******************************************************************************/
void user_init(void)
{
  print_system();

  if (init_uart(&user_config)) {
    printf("FATAL: init_uart\n");
    return;
  }

  if (init_logging(&user_config)) {
    printf("FATAL: init_logging\n");
    return;
  }

  if (init_spiffs()) {
    printf("FATAL: init_spiffs\n");
    return;
  }

  if (init_config(&user_config)) {
    printf("FATAL: init_config\n");
    return;
  }

  if (init_cli(&user_config)) {
    printf("FATAL: init_cli\n");
    return;
  }

  if (init_wifi(&user_config)) {
    printf("FATAL: init_wifi\n");
    return;
  } else {
    print_wifi();
  }
}

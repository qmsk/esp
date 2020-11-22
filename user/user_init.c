#include "user_config.h"
#include "spiffs.h"
#include "config.h"
#include "cli.h"
#include "wifi.h"
#include "dmx.h"
#include "artnet.h"
#include "spi.h"
#include "p9813.h"

#include <lib/logging.h>
#include <lib/uart.h>

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

void print_system_post()
{
  printf("*---------------------------------------\n");
  printf("|\tHeap free=%d\n", system_get_free_heap_size());
  printf("*---------------------------------------\n");
}

struct user_info user_info;

int init_uart(struct user_config *config)
{
  int err;

  UART_Config uart_config = {
    .baud_rate  = USER_CONFIG_UART_BAUD_RATE,
    .data_bits  = UART_WordLength_8b,
    .parity     = UART_Parity_None,
    .stop_bits  = UART_StopBits_1,
  };

  if ((err = uart_init(&uart0, UART_TX_QUEUE_SIZE, UART_RX_QUEUE_SIZE))) {
    LOG_ERROR("uart_init uart0");
    return err;
  }

  // TODO: move to dmx module?
  if ((err = uart_init(&uart1, UART_TX_QUEUE_SIZE, 0))) {
    LOG_ERROR("uart_init uart1");
    return err;
  }

  if ((err = uart_setup(&uart0, &uart_config))) {
    LOG_ERROR("uart_setup uart0");
    return err;
  }

  uart_disable(&uart1);

  uart_interrupts_enable();

  LOG_INFO("setup uart0 baud=%u", uart_config.baud_rate);

  return 0;
}

void user_init(void)
{
  print_system();

  if (init_uart(&user_config)) {
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

  if (init_config(&user_config)) {
    printf("FATAL: init_config\n");
    return;
  }

  if (init_cli(&user_config)) {
    printf("FATAL: init_cli\n");
    return;
  }

  if (init_wifi(&user_config.wifi, &user_info)) {
    printf("FATAL: init_wifi\n");
    return;
  }

  if (init_dmx(&user_config)) {
    printf("FATAL: init_dmx\n");
    return;
  }

  if (init_artnet(&user_config, &user_info)) {
    printf("FATAL: init_artnet\n");
    return;
  }

  if (init_p9813(&user_config)) {
    printf("FATAL: init_p9813\n");
    return;
  }

  print_system_post();
}

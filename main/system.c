#include "system.h"

#include <logging.h>
#include <system.h>

#include <esp_clk.h>
#include <esp_idf_version.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <spi_flash.h>

static int system_info_cmd(int argc, char **argv, void *ctx)
{
  esp_chip_info_t chip_info;
  esp_chip_info(&chip_info);

  system_image_info_t image_info;
  system_image_info(&image_info);

  printf("IDF version=%s\n", esp_get_idf_version());
  printf("CPU model=%#x features=%#x cores=%u revision=%u\n",
    chip_info.model,
    chip_info.features,
    chip_info.cores,
    chip_info.revision
  );
  printf("Flash size=%u\n", spi_flash_get_chip_size());
  printf("\n");
  printf("Firmware DRAM   start=%#08x end=%#08x usage=%8u size=%8u\n", (unsigned) image_info.dram_start, (unsigned) image_info.dram_end, image_info.dram_usage, image_info.dram_size);
  printf("Firmware IRAM   start=%#08x end=%#08x usage=%8u size=%8u\n", (unsigned) image_info.iram_start, (unsigned) image_info.iram_end, image_info.iram_usage, image_info.iram_size);
  printf("Firmware Flash  start=%#08x end=%#08x usage=%8u\n", (unsigned) image_info.flash_start, (unsigned) image_info.flash_end, image_info.flash_usage);
  printf("\n");
  printf("Heap DRAM   start=%#08x size=%d\n", (unsigned) image_info.dram_heap_start, image_info.dram_heap_size);
  printf("Heap IRAM   start=%#08x size=%d\n", (unsigned) image_info.iram_heap_start, image_info.iram_heap_size);

  return 0;
}

static int system_status_cmd(int argc, char **argv, void *ctx)
{
  esp_reset_reason_t reset_reason = esp_reset_reason();
  int uptime = esp_timer_get_time() / 1000 / 1000;

  printf("Uptime %ds\n", uptime);
  printf("Reset reason: %s\n", esp_reset_reason_str(reset_reason));
  printf("\n");
  printf("CPU frequency=%dMhz\n", esp_clk_cpu_freq() / 1000 / 1000);
  printf("\n");
  printf("Heap total=%8u free=%8u min_free=%u\n",
    system_get_total_heap_size(),
    esp_get_free_heap_size(),
    esp_get_minimum_free_heap_size()
  );

  return 0;
}

static int system_restart_cmd(int argc, char **argv, void *ctx)
{
  LOG_INFO("restarting...");

  esp_restart();
}

static const struct cmd system_commands[] = {
  { "info",     system_info_cmd,    .describe = "Print system info" },
  { "status",   system_status_cmd,  .describe = "Print system status" },
  { "restart",  system_restart_cmd, .describe = "Restart system" },
  {}
};

const struct cmdtab system_cmdtab = {
  .commands = system_commands,
};

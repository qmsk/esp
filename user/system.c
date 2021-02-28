#include <lib/cli.h>
#include <lib/logging.h>

#include <espressif/esp_system.h>
#include <espressif/spi_flash.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// symbols defined in linker scripts
extern char _text_start, _text_end;
extern char _irom0_text_start, _irom0_text_end;
extern char _data_start, _data_end;
extern char _rodata_start, _rodata_end;
extern char _bss_start, _bss_end;
extern char _heap_start;

// no linker symbol defined
#define HEAP_END ((char *) 0x40000000)
#define HEAP_SIZE (HEAP_END - &_heap_start)

// not exported
extern size_t xPortGetFreeHeapSize( void );
extern size_t xPortGetMinimumEverFreeHeapSize( void );

int system_cmd_info(int argc, char **argv, void *ctx)
{
  cli_printf("!\tSDK version %s\n", system_get_sdk_version());
  cli_printf("|\tESP8266	chip ID=0x%x\n",	system_get_chip_id());
  cli_printf("|\tBoot version=%d mode=%d addr=%d\n", system_get_boot_version(), system_get_boot_mode(), system_get_userbin_addr());
  cli_printf("|\tFlash id=%d size_map=%d\n", spi_flash_get_id(), system_get_flash_size_map());
  cli_printf("\n");
  cli_printf("!\tCode size: .text=%u .irom0.text=%u\n",
    (&_text_end - &_text_start),
    (&_irom0_text_end - &_irom0_text_start)
  );
  cli_printf("!\tData size: .data=%u .rodata=%u .bss=%u .heap=%u\n",
    (&_data_end - &_data_start),
    (&_rodata_end - &_rodata_start),
    (&_bss_end - &_bss_start),
    (HEAP_END - &_heap_start)
  );

  return 0;
}

int system_cmd_status(int argc, char **argv, void *ctx)
{
  struct rst_info *rst_info = system_get_rst_info();
  float vdd33 = ((float)system_get_vdd33()) / 1024;

  cli_printf("!\tReset reason=%d (exc cause=%u)\n", rst_info->reason, rst_info->exccause);
  cli_printf("|\tReset exc: cause=%u epc1=%08x epc2=%08x epc3=%08x excv=%08x depc=%08x rtn=%08x\n",
    rst_info->exccause,
    rst_info->epc1, rst_info->epc2, rst_info->epc3,
    rst_info->excvaddr,
    rst_info->depc,
    rst_info->rtn_addr
  );
  cli_printf("\n");
  cli_printf("|\tCPU freq=%d Mhz\n", system_get_cpu_freq());
  cli_printf("!\tPower: vdd=%f / 3.3 V", vdd33);
  cli_printf("\n");
  cli_printf("!\tMemory info:\n");
  cli_printf("|\tHeap size=%u free=%d min=%d\n",
    HEAP_SIZE,
    xPortGetFreeHeapSize(), /* system_get_free_heap_size */
    xPortGetMinimumEverFreeHeapSize()
  );

  return 0;
}

#if ( configUSE_TRACE_FACILITY == 1 )
int system_cmd_tasks(int argc, char **argv, void *ctx)
{
  unsigned count = uxTaskGetNumberOfTasks();
  unsigned long total_runtime;
  xTaskStatusType *tasks;
  int err = 0;

  if ((tasks = calloc(count, sizeof(*tasks)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((count = uxTaskGetSystemState(tasks, count, &total_runtime)) == 0) {
    LOG_ERROR("uxTaskGetSystemState");
    err = -1;
    goto error;
  }

  cli_printf("%4s %20s\t%1s %5s %s/%s %s\n", "ID", "NAME", "S", "PRI", "TIME", "STACK FREE");

  for (xTaskStatusType *task = tasks; task < tasks + count; task++) {
    cli_printf("%4u %20s\t%c %2u/%2u %u/%u %u\n");
  }

error:
  free(tasks);
  return err;
}
#endif

int system_cmd_restart(int argc, char **argv, void *ctx)
{
  LOG_INFO("restarting...");

  system_restart();

  return 0;
}

const struct cmd system_commands[] = {
  { "info",     system_cmd_info,    .describe = "Display system build info"   },
  { "status",   system_cmd_status,  .describe = "Display system runtime info" },
#if ( configUSE_TRACE_FACILITY == 1 )
  { "tasks",    system_cmd_tasks,   .describe = "Display system tasks info"   },
#endif
  { "restart",  system_cmd_restart, .describe = "Restart system"              },
  {}
};

const struct cmdtab system_cmdtab = {
  .commands = system_commands,
};

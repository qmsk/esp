#include "system.h"

#include <logging.h>
#include <system.h>
#include <system_partition.h>
#include <system_tasks.h>

#include <esp_partition.h>
#include <sdkconfig.h>

static int system_info_cmd(int argc, char **argv, void *ctx)
{
  struct system_info info;

  system_info_get(&info);

  printf("Chip:\n");
  printf("\tModel     %s\n", esp_chip_model_str(info.esp_chip_info.model));
  printf("\tFeatures: %#x\n", info.esp_chip_info.features);
  printf("\tRevision: %u\n", info.esp_chip_info.revision);
  printf("\n");
  printf("CPU:\n");
  printf("\tCores:   %u\n", info.esp_chip_info.cores);
  printf("\n");
  printf("SDK:\n");
  printf("\tName:    %s\n", info.esp_idf_name);
  printf("\tVersion: %s\n", info.esp_idf_version);
  printf("\n");
  printf("App:\n");
  printf("\tName:    %s\n", info.esp_app_desc->project_name);
  printf("\tVersion: %s\n", info.esp_app_desc->version);
  printf("\tBuild:   %s %s\n", info.esp_app_desc->date, info.esp_app_desc->time);

  return 0;
}

static int system_status_cmd(int argc, char **argv, void *ctx)
{
  struct system_status status;

  system_status_get(&status);

  printf("System:\n");
  printf("\tUptime: %u.%03us\n", status.uptime_s, status.uptime_us / 1000);
  printf("\tReset:  %s\n", esp_reset_reason_str(status.reset_reason));
  printf("\n");
  printf("CPU:\n");
  printf("\tFreq:   %6dMhz\n", status.cpu_frequency / 1000 / 1000);
  printf("\n");
  printf("Heap:\n");
  printf("\tTotal:  %6uK\n", status.total_heap_size / 1024);
  printf("\tFree:   %6uK\n", status.free_heap_size / 1024);
  printf("\tMin:    %6uK\n", status.minimum_free_heap_size / 1024);
  printf("\tMax:    %6uK\n", status.maximum_free_heap_size / 1024);

  return 0;
}

static int system_image_cmd(int argc, char **argv, void *ctx)
{
  struct system_image_info image_info;

  system_image_info_get(&image_info);

  printf("Flash size %6uK image %6uK\n", image_info.flash_size / 1024, image_info.flash_usage / 1024);
  printf("DRAM  size %6uK image %6uK heap %6uK\n", image_info.dram_total_size / 1024, image_info.dram_size / 1024, image_info.dram_heap_size / 1024);
  printf("IRAM  size %6uK image %6uK heap %6uK\n", image_info.iram_total_size / 1024, image_info.iram_size / 1024, image_info.iram_heap_size / 1024);
  printf("\n");
  printf("DROM  Image %#08x - %#08x = %6uK\n", image_info.drom_start, image_info.drom_end, image_info.drom_size / 1024);
  printf("DRAM  Image %#08x - %#08x = %6uK\n", image_info.dram_start, image_info.dram_end, image_info.dram_size / 1024);
  printf("DRAM  Heap  %#08x - %#08x = %6uK\n", image_info.dram_heap_start, image_info.dram_heap_end, image_info.dram_heap_size / 1024);
  printf("IRAM  Image %#08x - %#08x = %6uK\n", image_info.iram_start, image_info.iram_end, image_info.iram_size / 1024);
  printf("IRAM  Heap  %#08x - %#08x = %6uK\n", image_info.iram_heap_start, image_info.iram_heap_end, image_info.iram_heap_size / 1024);
  printf("IROM  Image %#08x - %#08x = %6uK\n", image_info.irom_start, image_info.irom_end, image_info.irom_size / 1024);

  return 0;
}

static int system_partitions_cmd(int argc, char **argv, void *ctx)
{
  esp_partition_iterator_t it;

  printf("%16s %4s %8s %8s %8s %8s %s\n", "NAME", "TYPE", "SUBTYPE", "START", "END", "SIZE", "FLAGS");

  for (it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL); it; it = esp_partition_next(it)) {
    const esp_partition_t *p = esp_partition_get(it);
    const char *type = esp_partition_type_str(p->type);
    const char *subtype = esp_partition_subtype_str(p->type, p->subtype);

    if (type && subtype) {
      printf("%16s %4s %8s %#08x %#08x %#08x %s\n",
        p->label,
        type,
        subtype,
        p->address,
        p->address + p->size,
        p->size,
        p->encrypted ? "E" : ""
      );
    } else if (type) {
      printf("%16s %4s %#08x %#08x %#08x %#08x %s\n",
        p->label,
        type,
        p->subtype,
        p->address,
        p->address + p->size,
        p->size,
        p->encrypted ? "E" : ""
      );
    } else {
      printf("%16s %#04x %#08x %#08x %#08x %#08x %s\n",
        p->label,
        p->type,
        p->subtype,
        p->address,
        p->address + p->size,
        p->size,
        p->encrypted ? "E" : ""
      );
    }
  }

  return 0;
}

static int system_tasks_cmd(int argc, char **argv, void *ctx)
{
  static struct system_tasks_state last, state;
  int err;

  if ((err = system_tasks_update(&state))) {
    LOG_ERROR("system_tasks_update");
    return -1;
  }

  printf("%4s %-20s %5s\t%3s\t%6s\t%6s\t%6s\t%6s\t%6s\n", "ID", "NAME", "STATE", "CPU", "PRI", "TOTAL%", "LAST%", "STACK", "STACK FREE");

  for (const TaskStatus_t *task = state.tasks; task < state.tasks + state.count; task++) {
    uint32_t total_usage = system_tasks_total_usage(&state, task);
    uint32_t last_usage = system_tasks_last_usage(&state, task, &last);

    printf("%4d %-20s %5c\t%3c\t%2d->%2d\t%3u.%-1u%%\t%3u.%-1u%%\t%6u\t%6u\n",
      task->xTaskNumber,
      task->pcTaskName,
      system_task_state_char(task->eCurrentState),
#if configTASKLIST_INCLUDE_COREID
      task->xCoreID == tskNO_AFFINITY ? ' ' : '0' + task->xCoreID,
#elif
      ' ',
#endif
      task->uxBasePriority, task->uxCurrentPriority,
      total_usage / 10, total_usage % 10,
      last_usage / 10, last_usage % 10,
      task->usStackSize,
      task->usStackHighWaterMark
    );
  }

  // swap
  system_tasks_release(&last);
  last = state;

  return 0;
}

static const struct cmd system_commands[] = {
  { "info",       system_info_cmd,        .describe = "Print system info" },
  { "image",      system_image_cmd,       .describe = "Print system image" },
  { "status",     system_status_cmd,      .describe = "Print system status" },
  { "partitions", system_partitions_cmd,  .describe = "Print system partitions" },
  { "tasks",      system_tasks_cmd  ,     .describe = "Print system tasks" },
  {}
};

const struct cmdtab system_cmdtab = {
  .commands = system_commands,
};

#include "system.h"

#include <logging.h>
#include <system.h>

#include <esp_clk.h>
#include <esp_idf_version.h>
#include <esp_system.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
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

static char system_task_state_char(eTaskState state)
{
    switch(state) {
      case eRunning:    return 'X';
      case eReady:      return 'R';
      case eBlocked:    return 'B';
      case eSuspended:  return 'S';
      case eDeleted:    return 'D';
      case eInvalid:    return 'I';
      default:          return '?';
    }
}

// store previous task counters for delta
static uint32_t last_tasks_runtime;
static unsigned last_tasks_count;
static struct system_task_counters {
  UBaseType_t xTaskNumber;
  uint32_t ulRunTimeCounter;
} *last_tasks_counters;

static struct system_task_counters *last_task_counters(UBaseType_t xTaskNumber)
{
  if (!last_tasks_counters) {
      return NULL;
  }

  for (struct system_task_counters *tc = last_tasks_counters; tc < last_tasks_counters + last_tasks_count; tc++) {
    if (tc->xTaskNumber == xTaskNumber) {
      return tc;
    }
  }

  return NULL;
}

static int system_tasks_cmd(int argc, char **argv, void *ctx)
{
  unsigned count = uxTaskGetNumberOfTasks();
  TaskStatus_t *tasks;
  uint32_t runtime;
  struct system_task_counters *tasks_counters;
  int err = 0;

  if (!(tasks = calloc(count, sizeof(*tasks)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (!(count = uxTaskGetSystemState(tasks, count, &runtime))) {
    err = -1;
    LOG_ERROR("uxTaskGetSystemState");
    goto error;
  }

  if (!(tasks_counters = calloc(count, sizeof(*tasks_counters)))) {
    LOG_ERROR("calloc");
    goto error;
  }

  printf("%4s %-20s %5s\t%6s\t%6s\t%6s\t%s\n", "ID", "NAME", "STATE", "PRI", "TOTAL%", "LAST%", "STACK FREE");

  unsigned total_runtime_permille = runtime / 1000; // for .x percentage calcuation
  unsigned last_runtime_permille = 0;

  if (last_tasks_runtime && runtime > last_tasks_runtime) {
    last_runtime_permille = (runtime - last_tasks_runtime) / 1000; // for .x percentage calcuation
  }

  for (unsigned i = 0; i < count; i++) {
    TaskStatus_t *task = &tasks[i];
    struct system_task_counters *next_counters = &tasks_counters[i];
    struct system_task_counters *last_counters = last_task_counters(task->xTaskNumber);

    unsigned total_usage = task->ulRunTimeCounter / total_runtime_permille;
    unsigned last_usage = 0;

    // calc last
    if (last_runtime_permille && last_counters && task->ulRunTimeCounter > last_counters->ulRunTimeCounter) {
      last_usage = (task->ulRunTimeCounter - last_counters->ulRunTimeCounter) / last_runtime_permille;
    }

    // update for next
    next_counters->xTaskNumber = task->xTaskNumber;
    next_counters->ulRunTimeCounter = task->ulRunTimeCounter;

    printf("%4d %-20s %c    \t%2d->%2d\t%3u.%-1u%%\t%3u.%-1u%%\t%u\n",
      task->xTaskNumber,
      task->pcTaskName,
      system_task_state_char(task->eCurrentState),
      task->uxBasePriority, task->uxCurrentPriority,
      total_usage / 10, total_usage % 10,
      last_usage / 10, last_usage % 10,
      task->usStackHighWaterMark
    );
  }

  // update for next
  if (!last_tasks_counters) {
    free(last_tasks_counters);
  }

  last_tasks_runtime = runtime;
  last_tasks_count = count;
  last_tasks_counters = tasks_counters;

error:
  free(tasks);

  return err;
}


static int system_restart_cmd(int argc, char **argv, void *ctx)
{
  LOG_INFO("restarting...");

  esp_restart();
}

static const struct cmd system_commands[] = {
  { "info",     system_info_cmd,    .describe = "Print system info" },
  { "status",   system_status_cmd,  .describe = "Print system status" },
  { "tasks",    system_tasks_cmd,   .describe = "Print system tasks" },
  { "restart",  system_restart_cmd, .describe = "Restart system" },
  {}
};

const struct cmdtab system_cmdtab = {
  .commands = system_commands,
};

#include "system.h"

#include <logging.h>
#include <system.h>
#include <system_tasks.h>
#include <system_interfaces.h>

#include <esp_partition.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

static int system_info_cmd(int argc, char **argv, void *ctx)
{
  struct system_info info;

  system_info_get(&info);

  printf("Chip model=%s features=%#x revision=%u\n",
    esp_chip_model_str(info.esp_chip_info.model),
    info.esp_chip_info.features,
    info.esp_chip_info.revision
  );
  printf("CPU cores=%u\n", info.esp_chip_info.cores);
  printf("\n");
  printf("SDK name=%s version=%s\n", info.esp_idf_name, info.esp_idf_version);
  printf("App name=%s version=%s\n", info.esp_app_desc.project_name, info.esp_app_desc.version);
  printf("Build date=%s time=%s\n", info.esp_app_desc.date, info.esp_app_desc.time);
  printf("\n");
  printf("Flash size=%8u usage=%8u\n", info.spi_flash_chip_size, info.image_info.flash_usage);
  printf("DRAM  size=%8u usage=%8u heap=%8u\n", info.image_info.dram_size, info.image_info.dram_usage, info.image_info.dram_heap_size);
  printf("IRAM  size=%8u usage=%8u heap=%8u\n", info.image_info.iram_size, info.image_info.iram_usage, info.image_info.iram_heap_size);

  return 0;
}

static int system_memory_cmd(int argc, char **argv, void *ctx)
{
  struct system_image_info image_info;

  system_image_info_get(&image_info);

  printf("DRAM  Image %#08x - %#08x = %8u\n", (unsigned) image_info.dram_start, (unsigned) image_info.dram_end, image_info.dram_usage);
  printf("DRAM  Heap  %#08x - %#08x = %8u\n", (unsigned) image_info.dram_heap_start, (unsigned) image_info.dram_heap_start + image_info.dram_heap_size, image_info.dram_heap_size);
  printf("IRAM  Image %#08x - %#08x = %8u\n", (unsigned) image_info.iram_start, (unsigned) image_info.iram_end, image_info.iram_usage);
  printf("IRAM  Heap  %#08x - %#08x = %8u\n", (unsigned) image_info.iram_heap_start, (unsigned) image_info.iram_heap_start + image_info.iram_heap_size, image_info.iram_heap_size);
  printf("Flash Image %#08x - %#08x = %8u\n", (unsigned) image_info.flash_start, (unsigned) image_info.flash_end, image_info.flash_usage);

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

static int system_status_cmd(int argc, char **argv, void *ctx)
{
  struct system_status status;

  system_status_get(&status);

  printf("Uptime %ds\n", (int) (status.uptime / 1000 / 1000));
  printf("Reset reason: %s\n", esp_reset_reason_str(status.reset_reason));
  printf("\n");
  printf("CPU frequency=%dMhz\n", status.cpu_frequency / 1000 / 1000);
  printf("\n");
  printf("Heap total=%8u free=%8u min_free=%u max_free=%u\n",
    status.total_heap_size,
    status.free_heap_size,
    status.minimum_free_heap_size,
    status.maximum_free_heap_size
  );

  return 0;
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

  printf("%4s %-20s %5s\t%6s\t%6s\t%6s\t%6s\t%6s\n", "ID", "NAME", "STATE", "PRI", "TOTAL%", "LAST%", "STACK", "STACK FREE");

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

    printf("%4d %-20s %5c\t%2d->%2d\t%3u.%-1u%%\t%3u.%-1u%%\t%6u\t%6u\n",
      task->xTaskNumber,
      task->pcTaskName,
      system_task_state_char(task->eCurrentState),
      task->uxBasePriority, task->uxCurrentPriority,
      total_usage / 10, total_usage % 10,
      last_usage / 10, last_usage % 10,
      task->usStackSize,
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

static inline void print_ip4_info(const char *title, ip4_addr_t *ip)
{
  printf("\t%-20s: %u.%u.%u.%u\n", title,
    ip4_addr1(ip),
    ip4_addr2(ip),
    ip4_addr3(ip),
    ip4_addr4(ip)
  );
}

static inline void print_ip4_cidr(const char *title, ip4_addr_t *ip, unsigned prefixlen)
{
  printf("\t%-20s: %u.%u.%u.%u/%u\n", title,
    ip4_addr1(ip),
    ip4_addr2(ip),
    ip4_addr3(ip),
    ip4_addr4(ip),
    prefixlen
  );
}

static inline void print_ip6_info(const char *title, ip6_addr_t *ip)
{
  printf("\t%-20s: %04x:%04x:%04x:%04x:%04x:%04x:%04x:%04x\n", title,
    IP6_ADDR_BLOCK1(ip),
    IP6_ADDR_BLOCK2(ip),
    IP6_ADDR_BLOCK3(ip),
    IP6_ADDR_BLOCK4(ip),
    IP6_ADDR_BLOCK5(ip),
    IP6_ADDR_BLOCK6(ip),
    IP6_ADDR_BLOCK7(ip),
    IP6_ADDR_BLOCK8(ip)
 );
}

static inline void print_ip_info(const char *title, ip_addr_t *ip)
{
  if (IP_IS_V4(ip)) {
    print_ip4_info(title, ip_2_ip4(ip));
  } else if (IP_IS_V6(ip)) {
    print_ip6_info(title, ip_2_ip6(ip));
  } else {
    LOG_WARN("invalid title=(%s) ip type=%d", title, IP_GET_TYPE(ip));
  }
}

int print_interface_info(tcpip_adapter_if_t tcpip_if)
{
  struct system_interface_info info;
  int err;

  if ((err = system_interface_info(&info, tcpip_if)) < 0) {
    LOG_ERROR("system_interface_info %s", tcpip_adapter_if_str(tcpip_if));
    return err;
  } else if (err > 0) {
    printf("TCP/IP %s: DOWN\n", tcpip_adapter_if_str(tcpip_if));
    return 0;
  }

  printf("TCP/IP %s: UP\n", tcpip_adapter_if_str(tcpip_if));
  printf("\t%-20s: %s\n", "Hostname", info.hostname ? info.hostname : "(null)");
  printf("\t%-20s: %s\n", "DHCP Server", tcpip_adapter_dhcp_status_str(info.dhcps_status));
  printf("\t%-20s: %s\n", "DHCP Client", tcpip_adapter_dhcp_status_str(info.dhcpc_status));
  print_ip4_info("IP", &info.ipv4.ip);
  print_ip4_info("Netmask", &info.ipv4.netmask);
  print_ip4_cidr("Network", &info.ipv4_network, info.ipv4_prefixlen);
  print_ip4_info("Gateway", &info.ipv4.gw);
  print_ip_info("DNS (main)", &info.dns_main.ip);
  print_ip_info("DNS (backup)", &info.dns_backup.ip);
  print_ip_info("DNS (fallback)", &info.dns_fallback.ip);

  return 0;
}

static int system_interfaces_cmd(int argc, char **argv, void *ctx)
{
  int err;

  if ((err = print_interface_info(TCPIP_ADAPTER_IF_STA))) {
    LOG_ERROR("tcpip_adapter_info");
    return err;
  }

  if ((err = print_interface_info(TCPIP_ADAPTER_IF_AP))) {
    LOG_ERROR("tcpip_adapter_info");
    return err;
  }

  return 0;
}

static int system_restart_cmd(int argc, char **argv, void *ctx)
{
  system_restart();
}

static const struct cmd system_commands[] = {
  { "info",       system_info_cmd,        .describe = "Print system info" },
  { "memory",     system_memory_cmd,      .describe = "Print system memory" },
  { "partitions", system_partitions_cmd,  .describe = "Print system partitions" },
  { "status",     system_status_cmd,      .describe = "Print system status" },
  { "tasks",      system_tasks_cmd,       .describe = "Print system tasks" },
  { "interfaces", system_interfaces_cmd,  .describe = "Print system network interfaces" },
  { "restart",    system_restart_cmd,     .describe = "Restart system" },
  {}
};

const struct cmdtab system_cmdtab = {
  .commands = system_commands,
};

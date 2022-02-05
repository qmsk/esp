#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>
#include <json_lwip.h>
#include <system.h>
#include <system_interfaces.h>
#include <system_partition.h>
#include <system_tasks.h>

#include <errno.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include <stdio.h>
#include <string.h>

static int system_api_write_info_object(struct json_writer *w)
{
  struct system_info info;
  struct system_image_info image_info;

  system_info_get(&info);
  system_image_info_get(&image_info);

  return (
    JSON_WRITE_MEMBER_STRING(w, "chip_model", esp_chip_model_str(info.esp_chip_info.model)) ||
    JSON_WRITE_MEMBER_UINT(w, "chip_revision", info.esp_chip_info.revision) ||
    JSON_WRITE_MEMBER_UINT(w, "cpu_cores", info.esp_chip_info.cores) ||
    JSON_WRITE_MEMBER_STRING(w, "sdk_name", info.esp_idf_name) ||
    JSON_WRITE_MEMBER_STRING(w, "sdk_version", info.esp_idf_version) ||
    JSON_WRITE_MEMBER_STRING(w, "app_name", info.esp_app_desc->project_name) ||
    JSON_WRITE_MEMBER_STRING(w, "app_version", info.esp_app_desc->version) ||
    JSON_WRITE_MEMBER_STRING(w, "build_date", info.esp_app_desc->date) ||
    JSON_WRITE_MEMBER_STRING(w, "build_time", info.esp_app_desc->time) ||
    JSON_WRITE_MEMBER_UINT(w, "flash_size", image_info.flash_size) ||
    JSON_WRITE_MEMBER_UINT(w, "flash_usage", image_info.flash_usage) ||
    JSON_WRITE_MEMBER_UINT(w, "iram_size", image_info.iram_size) ||
    JSON_WRITE_MEMBER_UINT(w, "iram_usage", image_info.iram_usage) ||
    JSON_WRITE_MEMBER_UINT(w, "iram_heap", image_info.iram_heap_size) ||
    JSON_WRITE_MEMBER_UINT(w, "dram_size", image_info.dram_size) ||
    JSON_WRITE_MEMBER_UINT(w, "dram_usage", image_info.dram_usage) ||
    JSON_WRITE_MEMBER_UINT(w, "dram_heap", image_info.dram_heap_size)
  );
}

static int system_api_write_status_object(struct json_writer *w)
{
  struct system_status status;

  system_status_get(&status);

  return (
    JSON_WRITE_MEMBER_UINT(w, "uptime_s", status.uptime_s) ||
    JSON_WRITE_MEMBER_UINT(w, "uptime_us", status.uptime_us) ||
    JSON_WRITE_MEMBER_STRING(w, "reset_reason", esp_reset_reason_str(status.reset_reason)) ||
    JSON_WRITE_MEMBER_UINT(w, "cpu_frequency", status.cpu_frequency) ||
    JSON_WRITE_MEMBER_UINT(w, "heap_size", status.total_heap_size) ||
    JSON_WRITE_MEMBER_UINT(w, "heap_free", status.free_heap_size) ||
    JSON_WRITE_MEMBER_UINT(w, "heap_free_min", status.minimum_free_heap_size) ||
    JSON_WRITE_MEMBER_UINT(w, "heap_free_max", status.maximum_free_heap_size)
  );
}

static int system_api_write_partition_object(struct json_writer *w, const esp_partition_t *p)
{
  const char *type = esp_partition_type_str(p->type);
  const char *subtype = esp_partition_subtype_str(p->type, p->subtype);

  return (
    JSON_WRITE_MEMBER_STRING(w, "label", p->label) ||
    (type ? JSON_WRITE_MEMBER_STRING(w, "type", type) : JSON_WRITE_MEMBER_INT(w, "type", p->type)) ||
    (subtype ? JSON_WRITE_MEMBER_STRING(w, "subtype", subtype) : JSON_WRITE_MEMBER_INT(w, "subtype", p->subtype)) ||
    JSON_WRITE_MEMBER_UINT(w, "start", p->address) ||
    JSON_WRITE_MEMBER_UINT(w, "end", p->address + p->size) ||
    JSON_WRITE_MEMBER_UINT(w, "size", p->size) ||
    JSON_WRITE_MEMBER_BOOL(w, "encrypted", p->encrypted)
  );
}

static int system_api_write_partitions_array(struct json_writer *w)
{
  int err;
  esp_partition_iterator_t it = NULL;

  for (it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL); it; it = esp_partition_next(it)) {
    const esp_partition_t *p = esp_partition_get(it);

    if ((err = JSON_WRITE_OBJECT(w, system_api_write_partition_object(w, p)))) {
      LOG_ERROR("system_api_write_partition_object");
      goto error;
    }
  }

error:
  if (it) {
    esp_partition_iterator_release(it);
  }

  return err;
}

static int system_api_write_task_object(struct json_writer *w, const TaskStatus_t *t, uint32_t total_runtime)
{
  return (
    JSON_WRITE_MEMBER_STRING(w, "name", t->pcTaskName) ||
    JSON_WRITE_MEMBER_UINT(w, "number", t->xTaskNumber) ||
    JSON_WRITE_MEMBER_STRING(w, "state", system_task_state_str(t->eCurrentState)) ||
    JSON_WRITE_MEMBER_UINT(w, "current_priority", t->uxCurrentPriority) ||
    JSON_WRITE_MEMBER_UINT(w, "base_priority", t->uxCurrentPriority) ||
    JSON_WRITE_MEMBER_UINT(w, "runtime", t->ulRunTimeCounter) ||
    JSON_WRITE_MEMBER_UINT(w, "total_runtime", total_runtime) ||
    JSON_WRITE_MEMBER_UINT(w, "stack_size", t->usStackSize) ||
    JSON_WRITE_MEMBER_UINT(w, "stack_highwater_mark", t->usStackHighWaterMark)
  );
}

static int system_api_write_tasks_array(struct json_writer *w)
{
  unsigned count = uxTaskGetNumberOfTasks();
  TaskStatus_t *tasks;
  uint32_t total_runtime;
  int err = 0;

  if (!(tasks = calloc(count, sizeof(*tasks)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if (!(count = uxTaskGetSystemState(tasks, count, &total_runtime))) {
    err = -1;
    LOG_ERROR("uxTaskGetSystemState");
    goto error;
  }

  for (unsigned i = 0; i < count; i++) {
    TaskStatus_t *t = &tasks[i];

    if ((err = JSON_WRITE_OBJECT(w, system_api_write_task_object(w, t, total_runtime)))) {
      LOG_ERROR("system_api_write_task_object");
      goto error;
    }
  }

error:
  free(tasks);

  return err;
}

static int system_api_write_interface_object(struct json_writer *w, tcpip_adapter_if_t tcpip_if)
{
  struct system_interface_info info;
  int err;

  if ((err = system_interface_info(&info, tcpip_if)) < 0) {
    LOG_ERROR("system_interface_info %s", tcpip_adapter_if_str(tcpip_if));
    return err;
  } else if (err > 0) {
    return JSON_WRITE_MEMBER_STRING(w, "state", "DOWN");
  }

  return (
    JSON_WRITE_MEMBER_STRING(w, "state", "UP") ||
    (info.hostname ? JSON_WRITE_MEMBER_STRING(w, "hostname", info.hostname) : JSON_WRITE_MEMBER_NULL(w, "hostname")) ||
    JSON_WRITE_MEMBER_STRING(w, "dhcp_server_status", tcpip_adapter_dhcp_status_str(info.dhcps_status)) ||
    JSON_WRITE_MEMBER_STRING(w, "dhcp_client_status", tcpip_adapter_dhcp_status_str(info.dhcpc_status)) ||
    JSON_WRITE_MEMBER_IPV4(w, "ipv4_address", &info.ipv4.ip) ||
    JSON_WRITE_MEMBER_IPV4(w, "ipv4_netmask", &info.ipv4.netmask) ||
    JSON_WRITE_MEMBER_IPV4(w, "ipv4_network", &info.ipv4_network) ||
    JSON_WRITE_MEMBER_UINT(w, "ipv4_prefixlen", info.ipv4_prefixlen) ||
    JSON_WRITE_MEMBER_IPV4(w, "ipv4_gateway", &info.ipv4.gw) ||
    JSON_WRITE_MEMBER_IP(w, "dns_main", &info.dns_main.ip) ||
    JSON_WRITE_MEMBER_IP(w, "dns_backup", &info.dns_backup.ip) ||
    JSON_WRITE_MEMBER_IP(w, "dns_fallback", &info.dns_fallback.ip)
  );
}

static int system_api_write_interfaces_object(struct json_writer *w)
{
  return (
    JSON_WRITE_MEMBER_OBJECT(w, "STA", system_api_write_interface_object(w, TCPIP_ADAPTER_IF_STA)) ||
    JSON_WRITE_MEMBER_OBJECT(w, "AP", system_api_write_interface_object(w, TCPIP_ADAPTER_IF_AP))
  );
}

static int system_api_write(struct json_writer *w, void *ctx)
{
  return JSON_WRITE_OBJECT(w,
    JSON_WRITE_MEMBER_OBJECT(w, "info", system_api_write_info_object(w)) ||
    JSON_WRITE_MEMBER_OBJECT(w, "status", system_api_write_status_object(w)) ||
    JSON_WRITE_MEMBER_ARRAY(w, "partitions", system_api_write_partitions_array(w)) ||
    JSON_WRITE_MEMBER_ARRAY(w, "tasks", system_api_write_tasks_array(w)) ||
    JSON_WRITE_MEMBER_OBJECT(w, "interfaces", system_api_write_interfaces_object(w))
  );
}

static int system_api_write_tasks(struct json_writer *w, void *ctx)
{
  return JSON_WRITE_ARRAY(w, system_api_write_tasks_array(w));
}

int system_api_handler(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = write_http_response_json(response, system_api_write, NULL))) {
    LOG_WARN("write_http_response_json -> system_api_write");
    return err;
  }

  return 0;
}

int system_api_tasks_handler(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = write_http_response_json(response, system_api_write_tasks, NULL))) {
    LOG_WARN("write_http_response_json -> system_api_write_tasks");
    return err;
  }

  return 0;
}

int system_api_restart_handler(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = http_response_start(response, HTTP_NO_CONTENT, NULL))) {
    LOG_WARN("http_response_start");
    return err;
  }

  if ((err = http_response_close(response))) {
    LOG_WARN("http_response_close");
    return err;
  }

  LOG_INFO("restarting...");

  // this function does not return
  esp_restart();
}

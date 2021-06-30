#include "http_routes.h"

#include <logging.h>
#include <json.h>
#include <system.h>

#include <errno.h>
#include <esp_system.h>
#include <stdio.h>
#include <string.h>

int system_api_write_info(struct json_writer *w)
{
  struct system_info info;

  system_info_get(&info);

  return (
    JSON_WRITE_MEMBER_STRING(w, "chip_model", esp_chip_model_str(info.esp_chip_info.model)) ||
    JSON_WRITE_MEMBER_UINT(w, "chip_revision", info.esp_chip_info.revision) ||
    JSON_WRITE_MEMBER_UINT(w, "cpu_cores", info.esp_chip_info.cores) ||
    JSON_WRITE_MEMBER_UINT(w, "flash_size", info.spi_flash_chip_size) ||
    JSON_WRITE_MEMBER_STRING(w, "sdk_version", info.esp_idf_version) ||
    JSON_WRITE_MEMBER_STRING(w, "app_name", info.esp_app_desc.project_name) ||
    JSON_WRITE_MEMBER_STRING(w, "app_version", info.esp_app_desc.version) ||
    JSON_WRITE_MEMBER_STRING(w, "build_date", info.esp_app_desc.date) ||
    JSON_WRITE_MEMBER_STRING(w, "build_time", info.esp_app_desc.time) ||
    JSON_WRITE_MEMBER_UINT(w, "iram_size", info.image_info.iram_size) ||
    JSON_WRITE_MEMBER_UINT(w, "iram_usage", info.image_info.iram_usage) ||
    JSON_WRITE_MEMBER_UINT(w, "iram_heap", info.image_info.iram_heap_size) ||
    JSON_WRITE_MEMBER_UINT(w, "dram_size", info.image_info.dram_size) ||
    JSON_WRITE_MEMBER_UINT(w, "dram_usage", info.image_info.dram_usage) ||
    JSON_WRITE_MEMBER_UINT(w, "dram_heap", info.image_info.dram_heap_size)
  );
}

int system_api_write_status(struct json_writer *w)
{
  struct system_status status;

  system_status_get(&status);

  return (
    JSON_WRITE_MEMBER_INT64(w, "uptime", status.uptime) ||
    JSON_WRITE_MEMBER_STRING(w, "reset_reason", esp_reset_reason_str(status.reset_reason)) ||
    JSON_WRITE_MEMBER_UINT(w, "cpu_frequency", status.cpu_frequency) ||
    JSON_WRITE_MEMBER_UINT(w, "heap_size", status.total_heap_size) ||
    JSON_WRITE_MEMBER_UINT(w, "heap_free", status.free_heap_size) ||
    JSON_WRITE_MEMBER_UINT(w, "heap_free_min", status.minimum_free_heap_size)
  );
}

int system_api_write(struct json_writer *w)
{
  return JSON_WRITE_OBJECT(w,
    JSON_WRITE_MEMBER_OBJECT(w, "info", system_api_write_info(w)) ||
    JSON_WRITE_MEMBER_OBJECT(w, "status", system_api_write_status(w))
  );
}

int system_api_handler(struct http_request *request, struct http_response *response, void *ctx)
{
  struct json_writer json_writer;
  FILE *file;
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = http_response_start(response, HTTP_OK, NULL))) {
    LOG_WARN("http_response_start");
    return err;
  }

  if ((err = http_response_header(response, "Content-Type", "application/json"))) {
    LOG_WARN("http_response_header");
    return err;
  }

  if ((err = http_response_open(response, &file))) {
    LOG_WARN("http_response_open");
    return err;
  }

  if (json_writer_init(&json_writer, file)) {
    LOG_ERROR("json_writer_init");
    return -1;
  }

  if ((err = system_api_write(&json_writer))) {
    LOG_WARN("config_get_api_write");
    return -1;
  }

  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
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

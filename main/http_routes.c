#include "http_routes.h"

const struct http_route http_routes[] = {
  /* http_dist.c */
  { "GET",  "",                   http_dist_index_handler, NULL },
  { "GET",  "dist/",              http_dist_handler,       NULL },

  /* config_http_file.c */
  { "GET",  "config.ini",         config_file_get,      NULL },
  { "POST", "config.ini",         config_file_post,     NULL },

  /* vfs_http.c */
  { "GET",  "vfs/",               vfs_http_get,         NULL },
  { "POST", "vfs/",               vfs_http_post,        NULL },

  /* config_http.c */
  { "GET",  "api/config",         config_api_get,       NULL },
  { "POST", "api/config",         config_api_post,      NULL },

  /* system_http.c */
  { "GET",  "api/system",         system_api_handler,         NULL },
  { "GET",  "api/system/tasks",   system_api_tasks_handler,   NULL },
  { "POST", "api/system/restart", system_api_restart_handler, NULL },

  /* wifi_http.c */
  { "GET",  "api/wifi",           wifi_api_handler,           NULL },
  { "POST", "api/wifi/scan",      wifi_api_scan_handler,      NULL },

  /* artnet_http.c */
  { "GET",  "api/artnet",         artnet_api_handler,         NULL },

  /* leds_http.c */
  { "GET",  "api/leds",           leds_api_get,           NULL },
  { "POST", "api/leds",           leds_api_post,          NULL },

  { "GET",  "api/leds/test",      leds_api_test_get,      NULL },
  { "POST", "api/leds/test",      leds_api_test_post,     NULL },
  {}
};

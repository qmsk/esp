#pragma once

#include <httpserver/handler.h>
#include <httpserver/router.h>

extern const struct http_route http_routes[];

/* http_dist.c */
int http_dist_handler(struct http_request *request, struct http_response *response, void *ctx);
int http_dist_index_handler(struct http_request *request, struct http_response *response, void *ctx);

/* vfs_http.c */
int vfs_http_get(struct http_request *request, struct http_response *response, void *ctx);
int vfs_http_put(struct http_request *request, struct http_response *response, void *ctx);
int vfs_http_post(struct http_request *request, struct http_response *response, void *ctx);
int vfs_http_delete(struct http_request *request, struct http_response *response, void *ctx);

/* config_http_file.c */
int config_file_get(struct http_request *request, struct http_response *response, void *ctx);
int config_file_post(struct http_request *request, struct http_response *response, void *ctx);

/* config_http_api.c */
int config_api_get(struct http_request *request, struct http_response *response, void *ctx);
int config_api_post(struct http_request *request, struct http_response *response, void *ctx);

/* system_http.c */
int system_api_handler(struct http_request *request, struct http_response *response, void *ctx);
int system_api_tasks_handler(struct http_request *request, struct http_response *response, void *ctx);
int system_api_restart_handler(struct http_request *request, struct http_response *response, void *ctx);

/* wifi_http.c */
int wifi_api_handler(struct http_request *request, struct http_response *response, void *ctx);
int wifi_api_scan_handler(struct http_request *request, struct http_response *response, void *ctx);

/* artnet_http.c */
int artnet_api_handler(struct http_request *request, struct http_response *response, void *ctx);
int artnet_api_inputs_handler(struct http_request *request, struct http_response *response, void *ctx);
int artnet_api_outputs_handler(struct http_request *request, struct http_response *response, void *ctx);

/* leds_http.c */
int leds_api_get(struct http_request *request, struct http_response *response, void *ctx);
int leds_api_post(struct http_request *request, struct http_response *response, void *ctx);

int leds_api_test_get(struct http_request *request, struct http_response *response, void *ctx);
int leds_api_test_post(struct http_request *request, struct http_response *response, void *ctx);

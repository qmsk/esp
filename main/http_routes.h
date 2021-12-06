#pragma once

#include <httpserver/handler.h>
#include <httpserver/router.h>

extern const struct http_route http_routes[];

/* http_dist.c */
int http_dist_handler(struct http_request *request, struct http_response *response, void *ctx);
int http_dist_index_handler(struct http_request *request, struct http_response *response, void *ctx);

/* config_http.c */
int config_get_handler(struct http_request *request, struct http_response *response, void *ctx);
int config_post_handler(struct http_request *request, struct http_response *response, void *ctx);

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

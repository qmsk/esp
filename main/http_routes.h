#pragma once

#include <httpserver/handler.h>
#include <httpserver/router.h>

extern const struct http_route http_routes[];

int http_index_handler(struct http_request *request, struct http_response *response, void *ctx);
//int config_get_handler(struct http_request *request, struct http_response *response, void *ctx);

/*
int config_api_get(struct http_request *request, struct http_response *response, void *ctx);
int config_api_post(struct http_request *request, struct http_response *response, void *ctx);
*/

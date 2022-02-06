#pragma once

#include <httpserver/handler.h>
#include <httpserver/router.h>

extern const struct http_route http_routes[];

/* http_dist.c */
int http_dist_handler(struct http_request *request, struct http_response *response, void *ctx);
int http_dist_index_handler(struct http_request *request, struct http_response *response, void *ctx);

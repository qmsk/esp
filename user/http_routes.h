#ifndef USER_HTTP_HANDLERS_H
#define USER_HTTP_HANDLERS_H

#include <lib/httpserver/handler.h>
#include <lib/httpserver/router.h>

extern const struct http_route http_routes[];

int http_index_handler(struct http_request *request, struct http_response *response, void *ctx);
int config_get_handler(struct http_request *request, struct http_response *response, void *ctx);
int config_post_handler(struct http_request *request, struct http_response *response, void *ctx);
int config_api_get(struct http_request *request, struct http_response *response, void *ctx);

#endif

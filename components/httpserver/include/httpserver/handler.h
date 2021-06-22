#ifndef HTTP_HANDLER_H
#define HTTP_HANDLER_H

#include "request.h"
#include "response.h"

typedef int (*http_handler_func)(struct http_request *request, struct http_response *response, void *ctx);

#endif

#include <httpserver/response.h>
#include <json.h>

int write_http_response_json(struct http_response *response, int(*f)(struct json_writer *w, void *ctx), void *ctx);

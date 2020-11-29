#ifndef HTTP_ROUTER_H
#define HTTP_ROUTER_H

#include "handler.h"

struct http_router;

/*
 * Request handler.
 */
struct http_route {
    // request matchers
    const char *method;
    const char *path;

    // handler
    http_handler_func handler;
    void *ctx;
};

int http_router_create (struct http_router **routerp);

/*
 * Add a server handler for requests.
 *
 * method   - NULL to accept any requests, or specific GET/POST/etc method to handle requests for.
 * path     -
 *      NULL    - wildcard match
 *      foo     - matches "GET /foo", but not "GET /foo/" nor "GET /foo/bar" nor "GET /foo.bar"
 *      foo/    - matches "GET /foo" and "GET /foo/" and "GET /foo/bar", but not "GET /foo.bar"
 *
 * The leading / in the path should be omitted.
 *
 * The given method/path must remain valid of the lifetime of the server.
 */
int http_router_add (struct http_router *router, const struct http_route *route);

/*
 * http_handler_func for http_router as ctx.
 */
int http_router_handler (struct http_request *request, struct http_response *response, void *ctx);

#endif

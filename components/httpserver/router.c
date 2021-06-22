#include "httpserver/router.h"

#include "logging.h"

#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

struct http_router {
    /* Handler lookup */
    TAILQ_HEAD(http_router_items, http_router_item) routes;
};

struct http_router_item {
    struct http_route route;

    TAILQ_ENTRY(http_router_item) router_routes;
};

int http_router_create (struct http_router **routerp)
{
    struct http_router *router = NULL;

    if (!(router = calloc(1, sizeof(*router)))) {
        LOG_ERROR("calloc");
        return -1;
    }

    TAILQ_INIT(&router->routes);

    *routerp = router;

    return 0;
}

int http_router_add (struct http_router *router, const struct http_route *route)
{
    struct http_router_item *item = NULL;

    if (!route) {
        LOG_ERROR("NULL route given");
        return -1;
    }

    if (!(item = calloc(1, sizeof(*item)))) {
        LOG_ERROR("calloc");
        return -1;
    }

    item->route = *route;

    TAILQ_INSERT_TAIL(&router->routes, item, router_routes);

    return 0;
}

/*
 * Match request path against handler.
 *
 * Returns 1 on mismatch, 0 on match.
 */
static int http_route_match_path (const struct http_route *route, const char *path)
{
    size_t pathlen = strlen(route->path), requestlen = strlen(path);

    if (!route->path) {
        // NULL path always matches
        return 0;
    }

    if (pathlen == 0) {
        // empty path only matches root
        return requestlen == 0 ? 0 : 1;
    }

    // handle trailing /
    size_t prefixlen = pathlen - 1;
    char endswith = route->path[prefixlen];

    if (endswith == '/') {
        // prefix match, but with optional trailing / in request
        if (requestlen == prefixlen) {
            // strict match, omitting trailing /
            return strncmp(route->path, path, prefixlen);
        } else {
            // prefix match, including trailing /
            return strncmp(route->path, path, pathlen);
        }
    } else {
        // exact match
        return strcmp(route->path, path);
    }
}

/*
 * Lookup a handler for the given request.
 */
static int http_router_lookup (struct http_router *router, const struct http_request *request, const struct http_route **routep)
{
    const char *method = http_request_method(request);
    const char *path = http_request_url(request)->path;
    struct http_router_item *item;
    enum http_status status = 404;

    TAILQ_FOREACH(item, &router->routes, router_routes) {
        const struct http_route *route = &item->route;

        if (route->path && http_route_match_path(route, path))
            continue;

        if (route->method && strcmp(route->method, method)) {
            // chain along so that a matching path but mismatching method is 405
            status = 405;
            continue;
        }

        LOG_DEBUG("%s %s", route->method, route->path);

        *routep = route;
        return 0;
    }

    LOG_WARN("%s: %d", path, status);
    return status;
}

int http_router_handler (struct http_request *request, struct http_response *response, void *ctx)
{
  struct http_router *router = ctx;
  const struct http_route *route;
  int err;

  // handler
  if ((err = http_router_lookup(router, request, &route)) < 0) {
      LOG_WARN("http_router_lookup");
      return err;

  } else if (err > 0) {
      return err;
  }

  return route->handler(request, response, route->ctx);
}

void http_router_destroy (struct http_router *router)
{
  for (struct http_router_item *item; (item = TAILQ_FIRST(&router->routes)); ) {
      TAILQ_REMOVE(&router->routes, item, router_routes);

      free(item);
  }
}

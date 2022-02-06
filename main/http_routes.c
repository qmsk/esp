#include "http_routes.h"

const struct http_route http_routes[] = {
  { "GET",  "",                http_dist_index_handler, NULL },
  { "GET",  "dist/",           http_dist_handler,       NULL },
  {}
};

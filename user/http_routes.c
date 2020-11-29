#include "http_routes.h"

const struct http_route http_routes[] = {
  { "GET", "",      http_index_handler, NULL },
  {}
};

#include "http.h"
#include "spiffs.h"

#include <logging.h>

#define HTTP_DIST_PARTITON_LABEL "web-dist"
#define HTTP_DIST_BASE_PATH "/web-dist"
#define HTTP_DIST_MAX_FILES 4

int init_http_dist()
{
  int err;

  if ((err = init_spiffs_partition(HTTP_DIST_BASE_PATH, HTTP_DIST_PARTITON_LABEL, HTTP_DIST_MAX_FILES)) < 0) {
    LOG_ERROR("init_spiffs_partition");
  } else if (err) {
    LOG_WARN("No configuration partition available");
    return 1;
  }

  return 0;
}

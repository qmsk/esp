#include "http.h"
#include "http_routes.h"

#include <httpserver/handler.h>
#include <logging.h>

#include <esp_err.h>
#include <esp_spiffs.h>

#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <sys/unistd.h>

#define HTTP_DIST_PARTITON_LABEL "web-dist"
#define HTTP_DIST_BASE_PATH "/web-dist"
#define HTTP_DIST_MAX_FILES 4

#define HTTP_DIST_MAX_PATH 64
#define HTTP_DIST_PUBLIC_PATH "dist/"

#define HTTP_DIST_INDEX_FILE "index.html"
#define HTTP_DIST_INDEX_PATH (HTTP_DIST_BASE_PATH "/" HTTP_DIST_INDEX_FILE)
#define HTTP_DIST_INDEX_CONTENT_TYPE "text/html"

static const struct http_dist_content_type {
  const char *ext;
  const char *content_type;
} http_dist_content_types[] = {
  { ".html",  "text/html" },
  { ".js",    "text/javascript" },
  { ".css",   "text/css" },
  { NULL,     "application/octet-stream" },
};

static const char *http_dist_content_type(const char *ext)
{
  const struct http_dist_content_type *t = http_dist_content_types;

  for (; t->ext; t++) {
    if (ext && strcmp(t->ext, ext) == 0) {
      break;
    }
  }

  return t->content_type;
}

static int http_dist_path(struct http_request *request, char *buf, size_t size, char **extp)
{
  const char *path = http_request_url(request)->path;
  const char *name = path;
  char *out = buf;
  *extp = NULL;

  // strip prefix
  for (const char *p = HTTP_DIST_PUBLIC_PATH; *p; p++) {
    if (*name && *name == *p) {
      name++;
    } else {
      LOG_WARN("Incorrect request path prefix: %s", path);
      return 404;
    }
  }

  // build path
  for (const char *p = HTTP_DIST_BASE_PATH; *p; p++) {
    if (out >= buf + size) {
      LOG_WARN("base-path overflow");
      return -1;
    }

    *out++ = *p;
  }

  if (out >= buf + size) {
    LOG_WARN("base-path overflow");
    return -1;
  }

  *out++ = '/';

  bool sep = true;

  for (const char *p = name; *p; p++) {
    if (out >= buf + size) {
      LOG_WARN("base-path overflow");
      return -1;
    }

    if (*p == '/') {
      sep = true;
    } else if (*p == '.') {
      if (sep) {
        LOG_WARN("Forbidden access to dot-file");
        return 403;
      } else {
        *extp = out;
      }
    } else {
      sep = false;
    }

    *out++ = *p;
  }

  if (out >= buf + size) {
    LOG_WARN("base-path overflow");
    return -1;
  }

  *out = '\0';

  return 0;
}

static int http_dist_response(struct http_response *response, const char *path, const char *content_type)
{
  int fd;
  int err;

  LOG_INFO("serve path=%s with content_type=%s", path, content_type);

  if ((fd = open(path, O_RDONLY, 0)) < 0) {
    if (errno = ENOENT) {
      LOG_WARN("not found: %s", path);
      return 404;
    } else {
      LOG_ERROR("open %s: %s", path, strerror(errno));
      return -1;
    }
  }

  if ((err = http_response_start(response, HTTP_OK, "OK"))) {
    LOG_ERROR("http_response_start");
    goto error;
  }

  if ((err = http_response_header(response, "Content-Type", "%s", content_type))) {
    LOG_ERROR("http_response_header");
    goto error;
  }

  if ((err = http_response_sendfile(response, fd, 0))) {
    LOG_ERROR("http_response_sendfile");
    goto error;
  }

error:
  close(fd);

  return err;
}

int http_dist_index_handler(struct http_request *request, struct http_response *response, void *ctx)
{
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  return http_dist_response(response, HTTP_DIST_INDEX_PATH, HTTP_DIST_INDEX_CONTENT_TYPE);
}

int http_dist_handler(struct http_request *request, struct http_response *response, void *ctx)
{
  char path[HTTP_DIST_MAX_PATH], *ext = NULL;
  int err;

  if ((err = http_request_headers(request, NULL))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = http_dist_path(request, path, sizeof(path), &ext))) {
    return err;
  }

  return http_dist_response(response, path, http_dist_content_type(ext));
}

int init_http_dist()
{
  esp_vfs_spiffs_conf_t conf = {
    .base_path              = HTTP_DIST_BASE_PATH,
    .partition_label        = HTTP_DIST_PARTITON_LABEL,
    .max_files              = HTTP_DIST_MAX_FILES,
    .format_if_mount_failed = true,
  };
  int err;

  LOG_INFO("mount/format partition=%s at base_path=%s with max_files=%u", conf.partition_label, conf.base_path, conf.max_files);

  if ((err = esp_vfs_spiffs_register(&conf))) {
    if (err == ESP_ERR_NOT_FOUND) {
      LOG_WARN("spiffs partition with label=%s not found", conf.partition_label);
    } else {
      LOG_ERROR("esp_vfs_spiffs_register: %s", esp_err_to_name(err));
    }
  }

  return 0;
}

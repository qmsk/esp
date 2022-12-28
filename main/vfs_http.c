#include "config.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define VFS_HTTP_URL_PATH "vfs/"

#define VFS_HTTP_PATH_PREFIX "/config/"
#define VFS_HTTP_PATH_MAX 64

#define VFS_HTTP_CONTENT_TYPE "application/octet-stream"

struct vfs_http_params {
  char path[VFS_HTTP_PATH_MAX];
};

static int vfs_http_params(struct http_request *request, struct vfs_http_params *params)
{
  const struct url *url = http_request_url(request);
  const char *path = url->path;
  int ret;

  // strip http prefix
  for (const char *p = VFS_HTTP_URL_PATH; *p; p++) {
    if (*path && *path == *p) {
      path++;
    } else {
      LOG_WARN("Incorrect request path prefix: %s", url->path);
      return 404;
    }
  }

  if (!path[0]) {
    LOG_WARN("Missing path=%s", params->path);

    return HTTP_UNPROCESSABLE_ENTITY;
  }

  // add vfs prefix
  ret = snprintf(params->path, sizeof(params->path), "%s%s", VFS_HTTP_PATH_PREFIX, path);

  if (ret < 0) {
    LOG_ERROR("snprintf");
    return -1;
  } else if (ret >= sizeof(params->path)) {
    LOG_WARN("path len=%d", ret);
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  return 0;
}

static int vfs_http_open(FILE **filep, const struct vfs_http_params *params, const char *mode)
{
  if ((*filep = fopen(params->path, mode))) {
    return 0;
  } else if (errno == ENOENT) {
    LOG_WARN("fopen: %s", strerror(errno));
    return HTTP_NOT_FOUND;
  } else {
    LOG_ERROR("fopen: %s", strerror(errno));
    return -1;
  }
}

static int vfs_http_unlink(const struct vfs_http_params *params)
{
  if (unlink(params->path)) {
    if (errno == ENOENT) {
      LOG_WARN("unlink: %s", strerror(errno));
      return HTTP_NOT_FOUND;
    } else {
      LOG_ERROR("unlink: %s", strerror(errno));
      return -1;
    }
  }

  return 0;
}

static int vfs_copy(FILE *read, FILE *write)
{
  int c;
  size_t size = 0;

  LOG_DEBUG("read=%p, write=%p", read, write);

  while ((c = fgetc(read)) != EOF) {
    if (fputc(c, write) == EOF) {
      LOG_ERROR("fputc: %s", strerror(errno));
      return -1;
    }

    size++;
  }

  LOG_INFO("size=%u", size);

  if (ferror(read)) {
    LOG_ERROR("fgetc: %s", strerror(errno));
    return -1;
  }

  return 0;
}

static int vfs_http_read(struct http_response *response, const struct vfs_http_params *params)
{
  FILE *http_file, *file;
  int err;

  LOG_INFO("path=%s", params->path);

  if ((err = vfs_http_open(&file, params, "r")) < 0) {
    LOG_ERROR("vfs_http_open");
    return err;
  } else if (err) {
    LOG_WARN("vfs_http_open: %d", err);
    return err;
  }

  if ((err = http_response_start(response, HTTP_OK, NULL))) {
    LOG_WARN("http_response_start");
    goto error;
  }

  if ((err = http_response_header(response, "Content-Type", "%s", VFS_HTTP_CONTENT_TYPE))) {
    LOG_WARN("http_response_header");
    goto error;
  }

  if ((err = http_response_header(response, "Content-Disposition", "attachment; filename=\"%s\"", params->path))) {
    LOG_WARN("http_response_header");
    goto error;
  }

  if ((err = http_response_open(response, &http_file))) {
    LOG_WARN("http_response_open");
    goto error;
  }

  if ((err = vfs_copy(file, http_file))) {
    LOG_ERROR("vfs_copy");
    goto file_error;
  }

file_error:
  if (fclose(http_file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

error:
  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

  return err;
}

static int vfs_http_write(struct http_request *request, const struct vfs_http_params *params)
{
  FILE *http_file, *file;
  int err;

  LOG_INFO("path=%s", params->path);

  if ((err = vfs_http_open(&file, params, "w")) < 0) {
    LOG_ERROR("vfs_http_open");
    return err;
  } else if (err) {
    LOG_WARN("vfs_http_open: %d", err);
    return err;
  }

  if ((err = http_request_open(request, &http_file))) {
    LOG_WARN("http_request_open");
    goto error;
  }

  if ((err = vfs_copy(http_file, file))) {
    LOG_ERROR("vfs_copy");
    goto file_error;
  }

file_error:
  if (fclose(http_file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

error:
  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    return -1;
  }

  return err;
}

int vfs_http_get(struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  struct vfs_http_params params = {};
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = vfs_http_params(request, &params))) {
    LOG_WARN("vfs_http_params");
    return err;
  }

  if ((err = vfs_http_read(response, &params))) {
    LOG_WARN("vfs_http_read");
    return err;
  }

  return 0;
}

int vfs_http_post(struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  struct vfs_http_params params = {};
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = vfs_http_params(request, &params))) {
    LOG_WARN("vfs_http_params");
    return err;
  }

  if ((err = vfs_http_write(request, &params))) {
    LOG_WARN("vfs_http_write");
    return err;
  }

  return err;
}

int vfs_http_delete(struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  struct vfs_http_params params = {};
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  if ((err = vfs_http_params(request, &params))) {
    LOG_WARN("vfs_http_params");
    return err;
  }

  if ((err = vfs_http_unlink(&params))) {
    LOG_WARN("vfs_http_unlink");
    return err;
  }

  return err;
}

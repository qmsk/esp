#include "config.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#define VFS_HTTP_URL_PATH "vfs/"

#define VFS_HTTP_PATH_MAX 64

#define VFS_HTTP_CONTENT_TYPE "application/octet-stream"

static const struct vfs_http_mount {
  const char *path;
} vfs_http_mounts[] = {
  { "/config" },
  {},
};

static int vfs_http_mount_match(const struct vfs_http_mount *mount, const char *path)
{
  const char *p = path, *m = mount->path;

  while (*p && *p++ == *m++ && *m) {

  }

  if (!*p) {
    if (!*m) {
      // exact match
      return 1;
    } else {
      // short
      return 0;
    }
  } else if (!*m) {
    if (*p == '/') {
      // directory prefix match
      return 1;
    } else {
      return 0;
    }
  } else {
    return 0;
  }
}

enum vfs_http_type {
  VFS_HTTP_TYPE_ROOT,
  VFS_HTTP_TYPE_MOUNT,
  VFS_HTTP_TYPE_DIRECTORY,
  VFS_HTTP_TYPE_FILE,
};

struct vfs_http_params {
  enum vfs_http_type type;
  const struct vfs_http_mount *mount;
  char path[VFS_HTTP_PATH_MAX];
};

static int vfs_http_params(struct http_request *request, struct vfs_http_params *params)
{
  const struct url *url = http_request_url(request);
  const char *path = url->path;
  int ret;

  // strip http url prefix, optional / suffix, leave / prefix
  for (const char *p = VFS_HTTP_URL_PATH; *p; p++) {
    if (*path == '/' && p[0] == '/' && !p[1]) {
      break;
    } else if (*path && *path == *p) {
      path++;
    } else if (p[0] == '/' && !p[1] && !*path) {
      break;
    } else {
      LOG_WARN("Incorrect url path prefix: %s", url->path);
      return HTTP_NOT_FOUND;
    }
  }

  if (!*path || (path[0] == '/' && !path[1])) {
    params->type = VFS_HTTP_TYPE_ROOT;

    return 0;
  }

  // match VFS mount
  for (const struct vfs_http_mount *m = vfs_http_mounts; m->path; m++) {
    if (vfs_http_mount_match(m, path)) {
      params->mount = m;
    }
  }

  if (!params->mount) {
    LOG_WARN("No matching path=%s prefix", params->path);

    return HTTP_NOT_FOUND;
  }

  // add vfs prefix
  ret = snprintf(params->path, sizeof(params->path), "%s", path);

  if (ret < 0) {
    LOG_ERROR("snprintf");
    return -1;
  } else if (ret >= sizeof(params->path)) {
    LOG_WARN("path len=%d", ret);
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  if (!path[0]) {
    params->type = VFS_HTTP_TYPE_ROOT;
  } else if (strcmp(path, params->mount->path) == 0) {
    params->type = VFS_HTTP_TYPE_MOUNT;
  } else if (path[strlen(path) - 1] == '/') {
    params->path[strlen(params->path) - 1] = '\0';
    params->type = VFS_HTTP_TYPE_DIRECTORY;
  } else {
    params->type = VFS_HTTP_TYPE_FILE;
  }

  return 0;
}

static int vfs_http_open(FILE **filep, const struct vfs_http_params *params, const char *mode)
{
  if (params->type != VFS_HTTP_TYPE_FILE) {
    return HTTP_UNPROCESSABLE_ENTITY;
  }

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
  if (params->type != VFS_HTTP_TYPE_FILE) {
    return HTTP_UNPROCESSABLE_ENTITY;
  }

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

static const char *vfs_http_api_dirent_type(const struct dirent *d)
{
  switch(d->d_type) {
    case DT_REG:  return "file";
    case DT_DIR:  return "directory";
    default:      return "unknown";
  }
}

static int vfs_http_api_write_directory_file_object(struct json_writer *w, const char *path, const struct dirent *d)
{
  char buf[256];
  struct stat st = {};
  struct tm tm;
  char tm_buf[20];
  int err = 0;

  err |= JSON_WRITE_MEMBER_STRING(w, "name", d->d_name);
  err |= JSON_WRITE_MEMBER_STRING(w, "type", vfs_http_api_dirent_type(d));

  if (d->d_type != DT_REG) {

  } else if (snprintf(buf, sizeof(buf), "%s/%s", path, d->d_name) >= sizeof(buf)) {
    LOG_WARN("path=%s overflow d_name=%s", path, d->d_name);
  } else if (stat(buf, &st)) {
    LOG_WARN("stat %s: %s", buf, strerror(errno));
  } else {
    localtime_r(&st.st_mtime, &tm);
    strftime(tm_buf, sizeof(tm_buf), "%F %T", &tm);

    err |= JSON_WRITE_MEMBER_INT(w, "size", st.st_size);
    err |= JSON_WRITE_MEMBER_STRING(w, "mtime", tm_buf);
  }

  return err;
}

static int vfs_http_api_write_directory_files(struct json_writer *w, const char *path)
{
  DIR *dir;
  struct dirent *d;
  int err = 0;

  if (!(dir = opendir(path))) {
    LOG_ERROR("opendir %s: %s", path, strerror(errno));
    return -1;
  }

  while ((d = readdir(dir))) {
    if ((err = JSON_WRITE_OBJECT(w, vfs_http_api_write_directory_file_object(w, path, d)))) {
      return err;
    }
  }

  closedir(dir);

  return err;
}

static int vfs_http_api_write_directory(struct json_writer *w, void *ctx)
{
  const char *path = ctx;

  return JSON_WRITE_ARRAY(w, vfs_http_api_write_directory_files(w, path));
}

static int vfs_http_api_write_mount_object(struct json_writer *w, const struct vfs_http_mount *mount)
{
  return JSON_WRITE_OBJECT(w,
        JSON_WRITE_MEMBER_STRING(w, "path", mount->path)
    ||  JSON_WRITE_MEMBER_ARRAY(w, "files", vfs_http_api_write_directory_files(w, mount->path))
  );
}

static int vfs_http_api_write_mount(struct json_writer *w, void *ctx)
{
  const struct vfs_http_mount *mount = ctx;

  return vfs_http_api_write_mount_object(w, mount);
}

static int vfs_http_api_write_root_mounts(struct json_writer *w)
{
  int err;

  for (const struct vfs_http_mount *m = vfs_http_mounts; m->path; m++) {
    if ((err = vfs_http_api_write_mount_object(w, m))) {
      return err;
    }
  }

  return 0;
}

static int vfs_http_api_write_root(struct json_writer *w, void *ctx)
{
  return JSON_WRITE_ARRAY(w, vfs_http_api_write_root_mounts(w));
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

  switch (params.type) {
    case VFS_HTTP_TYPE_ROOT:
      if ((err = write_http_response_json(response, vfs_http_api_write_root, NULL))) {
        LOG_WARN("write_http_response_json -> vfs_http_api_write_root");
        return err;
      }

      break;

    case VFS_HTTP_TYPE_MOUNT:
      if ((err = write_http_response_json(response, vfs_http_api_write_mount, (void *) params.mount))) {
        LOG_WARN("write_http_response_json -> vfs_http_api_write_mount");
        return err;
      }

      break;

    case VFS_HTTP_TYPE_DIRECTORY:
      if ((err = write_http_response_json(response, vfs_http_api_write_directory, params.path))) {
        LOG_WARN("write_http_response_json -> vfs_http_api_write_directory");
        return err;
      }

      break;

    case VFS_HTTP_TYPE_FILE:
      if ((err = vfs_http_read(response, &params))) {
        LOG_WARN("vfs_http_read");
        return err;
      }

      break;

    default:
      return HTTP_UNPROCESSABLE_ENTITY;
  }


  return 0;
}

int vfs_http_put(struct http_request *request, struct http_response *response, void *ctx)
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

#include "config.h"
#include "http_routes.h"
#include "http_handlers.h"
#include "sdcard.h"

#include <logging.h>

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <utime.h>

#define VFS_HTTP_URL_PATH "vfs/"

#define VFS_HTTP_PATH_SIZE 64

#define VFS_HTTP_CONTENT_TYPE "application/octet-stream"

#define VFS_HTTP_MKDIR_MODE 0775

static const struct vfs_http_mount {
  const char *path;
} vfs_http_mounts[] = {
  { "/config" },
#if CONFIG_SDCARD_ENABLED
  { "/sdcard" },
#endif
  {},
};

static const char *vfs_http_mount_match(const struct vfs_http_mount *mount, const char *path)
{
  const char *p = path, *m = mount->path;

  // scan common prefix
  while (*p && *p++ == *m++ && *m) {

  }

  if (!*p) {
    if (!*m) {
      // exact match
      return p;
    } else {
      // short
      return NULL;
    }
  } else if (!*m) {
    if (*p == '/') {
      // directory prefix match
      return p + 1;
    } else {
      return false;
    }
  } else {
    return false;
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
  const char *name;
  time_t mtime;

  char path[VFS_HTTP_PATH_SIZE];
  DIR *dir;
};

static int vfs_http_params(struct http_request *request, struct vfs_http_params *params)
{
  const struct http_request_headers *headers;
  const struct url *url = http_request_url(request);
  const char *path = url->path;
  int ret;

  // read headers
  if ((ret = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return ret;
  }

  if (headers->last_modified) {
    params->mtime = headers->last_modified;
  }

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

  // full VFS path
  ret = snprintf(params->path, sizeof(params->path), "%s", path);

  if (ret < 0) {
    LOG_ERROR("snprintf");
    return -1;
  } else if (ret >= sizeof(params->path)) {
    LOG_WARN("path len=%d", ret);
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  if (!*path || (path[0] == '/' && !path[1])) {
    params->type = VFS_HTTP_TYPE_ROOT;

    return 0;
  }

  // match VFS mount
  const char *name = NULL;

  for (const struct vfs_http_mount *m = vfs_http_mounts; m->path; m++) {
    if ((name = vfs_http_mount_match(m, path))) {
      params->mount = m;
      params->name = name;

      break;
    }
  }

  if (!name) {
    LOG_WARN("No matching path=%s mount", params->path);

    return HTTP_NOT_FOUND;
  }

  // format filesystem path
  if (strcmp(path, params->mount->path) == 0) {
    params->type = VFS_HTTP_TYPE_MOUNT;
  } else if (path[strlen(path) - 1] == '/') {
    params->path[strlen(params->path) - 1] = '\0';
    params->type = VFS_HTTP_TYPE_DIRECTORY;
  } else {
    params->type = VFS_HTTP_TYPE_FILE;
  }

  return 0;
}

static int vfs_http_error(const char *op, const char *path)
{
  switch(errno) {
    case ENOENT:
      LOG_WARN("%s %s: %s", op, path, strerror(errno));
      return HTTP_NOT_FOUND;

    case ENOTSUP:
      LOG_WARN("%s %s: %s", op, path, strerror(errno));
      return HTTP_METHOD_NOT_ALLOWED;

    case EACCES:
      LOG_WARN("%s %s: %s", op, path, strerror(errno));
      return HTTP_FORBIDDEN;

    default:
      LOG_ERROR("%s %s: %s", op, path, strerror(errno));
      return HTTP_INTERNAL_SERVER_ERROR;
  }
}

static int vfs_http_open(FILE **filep, const struct vfs_http_params *params, const char *mode)
{
  if (params->type != VFS_HTTP_TYPE_FILE) {
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  if ((*filep = fopen(params->path, mode))) {
    return 0;
  } else {
    return vfs_http_error("fopen", params->path);
  }
}

static int vfs_http_opendir(struct vfs_http_params *params)
{
  if (params->type != VFS_HTTP_TYPE_DIRECTORY && params->type != VFS_HTTP_TYPE_MOUNT) {
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  if ((params->dir = opendir(params->path))) {
    return 0;
  } else {
    return vfs_http_error("opendir", params->path);
  }
}

static int vfs_http_mkdir(const struct vfs_http_params *params)
{
  if (params->type != VFS_HTTP_TYPE_DIRECTORY) {
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  LOG_INFO("%s", params->path);

  if (mkdir(params->path, VFS_HTTP_MKDIR_MODE)) {
    return vfs_http_error("mkdir", params->path);
  }

  return 0;
}

static int vfs_http_rmdir(const struct vfs_http_params *params)
{
  if (params->type != VFS_HTTP_TYPE_DIRECTORY) {
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  LOG_INFO("%s", params->path);

  if (rmdir(params->path)) {
    return vfs_http_error("rmdir", params->path);
  }

  return 0;
}

static int vfs_http_unlink(const struct vfs_http_params *params)
{
  if (params->type != VFS_HTTP_TYPE_FILE) {
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  LOG_INFO("%s", params->path);

  if (unlink(params->path)) {
    return vfs_http_error("unlink", params->path);
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
  int err = 0;

  LOG_INFO("path=%s mtime=%ld", params->path, params->mtime);

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
    err = -1;
  }

error:
  if (fclose(file) < 0) {
    LOG_WARN("fclose: %s", strerror(errno));
    err = -1;
  }

  if (params->mtime && !err) {
    struct utimbuf times = {
      .actime = 0,
      .modtime = params->mtime,
    };

    if (utime(params->path, &times)) {
      LOG_WARN("utime: %s", strerror(errno));
    }
  }

  return err;
}

static int vfs_http_api_write_file_object(struct json_writer *w, const char *name, const char *path)
{
  struct stat st = {};
  struct tm tm;
  char tm_buf[20];
  int err = 0;

  err |= JSON_WRITE_MEMBER_STRING(w, "name", name);
  err |= JSON_WRITE_MEMBER_STRING(w, "type", "file");

  if (stat(path, &st)) {
    LOG_WARN("stat %s: %s", path, strerror(errno));
  } else {
    localtime_r(&st.st_mtime, &tm);
    strftime(tm_buf, sizeof(tm_buf), "%F %T", &tm);

    err |= JSON_WRITE_MEMBER_INT(w, "size", st.st_size);
    err |= JSON_WRITE_MEMBER_STRING(w, "mtime", tm_buf);
  }

  return err;
}

static int vfs_http_api_write_file(struct json_writer *w, void *ctx)
{
  const struct vfs_http_params *params = ctx;

  return JSON_WRITE_OBJECT(w, vfs_http_api_write_file_object(w, params->name, params->path));
}

static int vfs_http_api_write_directory_object(struct json_writer *w, const char *name)
{
  int err = 0;

  err |= JSON_WRITE_MEMBER_STRING(w, "name", name);
  err |= JSON_WRITE_MEMBER_STRING(w, "type", "directory");

  return err;
}

static int vfs_http_api_write_dirent_object(struct json_writer *w, const char *path, struct dirent *d)
{
  char buf[VFS_HTTP_PATH_SIZE];
  int err;

  if (snprintf(buf, sizeof(buf), "%s/%s", path, d->d_name) >= sizeof(buf)) {
    LOG_WARN("path=%s overflow d_name=%s", path, d->d_name);
  }

  switch (d->d_type) {
    case DT_REG:
      if ((err = vfs_http_api_write_file_object(w, d->d_name, buf))) {
        return err;
      }
      break;

    case DT_DIR:
      if ((err = vfs_http_api_write_directory_object(w, d->d_name))) {
        return err;
      }
      break;

    default:
      LOG_WARN("unkonwn d_type=%d d_name=%s", d->d_type, d->d_name);
  }

  return 0;
}

static int vfs_http_api_write_directory_array(struct json_writer *w, DIR *dir, const char *path)
{
  struct dirent *d;
  int err = 0;

  while ((d = readdir(dir))) {
    if ((err = JSON_WRITE_OBJECT(w, vfs_http_api_write_dirent_object(w, path, d)))) {
      goto error;
    }
  }

error:
  closedir(dir);

  return err;
}

static int vfs_http_api_write_directory(struct json_writer *w, void *ctx)
{
  const struct vfs_http_params *params = ctx;

  return JSON_WRITE_OBJECT(w,
        vfs_http_api_write_directory_object(w, params->name)
    ||  (params->dir ? JSON_WRITE_MEMBER_ARRAY(w, "files", vfs_http_api_write_directory_array(w, params->dir, params->path)) : 0)
  );
}

static int vfs_http_api_write_mount_object(struct json_writer *w, const struct vfs_http_mount *mount, DIR *dir)
{
  return JSON_WRITE_OBJECT(w,
        JSON_WRITE_MEMBER_STRING(w, "path", mount->path)
    ||  (dir ? JSON_WRITE_MEMBER_ARRAY(w, "files", vfs_http_api_write_directory_array(w, dir, mount->path)) : 0)
  );
}

static int vfs_http_api_write_mount(struct json_writer *w, void *ctx)
{
  const struct vfs_http_params *params = ctx;

  return vfs_http_api_write_mount_object(w, params->mount, params->dir);
}

static int vfs_http_api_write_root_mounts(struct json_writer *w)
{
  DIR *dir;
  int err;

  for (const struct vfs_http_mount *m = vfs_http_mounts; m->path; m++) {
    if (!(dir = opendir(m->path))) {
      LOG_WARN("opendir %s: %s", m->path, strerror(errno));
    }

    if ((err = vfs_http_api_write_mount_object(w, m, dir))) {
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
      if ((err = vfs_http_opendir(&params))) {
        LOG_WARN("vfs_http_opendir");
        return err;
      }

      if ((err = write_http_response_json(response, vfs_http_api_write_mount, &params))) {
        LOG_WARN("write_http_response_json -> vfs_http_api_write_mount");
        return err;
      }

      break;

    case VFS_HTTP_TYPE_DIRECTORY:
      if ((err = vfs_http_opendir(&params))) {
        LOG_WARN("vfs_http_opendir");
        return err;
      }

      if ((err = write_http_response_json(response, vfs_http_api_write_directory, &params))) {
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

  if ((err = write_http_response_json(response, vfs_http_api_write_file, &params))) {
    LOG_WARN("write_http_response_json -> vfs_http_api_write_file");
    return err;
  }

  return err;
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

  switch (params.type) {
    case VFS_HTTP_TYPE_ROOT:
      return HTTP_METHOD_NOT_ALLOWED;

    case VFS_HTTP_TYPE_MOUNT:
      return HTTP_METHOD_NOT_ALLOWED;

    case VFS_HTTP_TYPE_DIRECTORY:
      if ((err = vfs_http_mkdir(&params))) {
        LOG_WARN("vfs_http_mkdir");
        return err;
      }

      if ((err = write_http_response_json(response, vfs_http_api_write_directory, &params))) {
        LOG_WARN("write_http_response_json -> vfs_http_api_write_directory");
        return err;
      }

      break;

    case VFS_HTTP_TYPE_FILE:
      return HTTP_METHOD_NOT_ALLOWED;

    default:
      return HTTP_UNPROCESSABLE_ENTITY;
  }

  return 0;
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

  switch (params.type) {
    case VFS_HTTP_TYPE_ROOT:
      return HTTP_METHOD_NOT_ALLOWED;

    case VFS_HTTP_TYPE_MOUNT:
      return HTTP_METHOD_NOT_ALLOWED;

    case VFS_HTTP_TYPE_DIRECTORY:
      if ((err = vfs_http_rmdir(&params))) {
        LOG_WARN("vfs_http_rmdir");
        return err;
      }

      break;

    case VFS_HTTP_TYPE_FILE:
      if ((err = vfs_http_unlink(&params))) {
        LOG_WARN("vfs_http_unlink");
        return err;
      }

      break;

    default:
      return HTTP_UNPROCESSABLE_ENTITY;
  }

  return err;
}

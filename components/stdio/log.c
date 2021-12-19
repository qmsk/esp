#include <stdio_log.h>
#include "log.h"

#include <logging.h>

#include <stdlib.h>
#include <string.h>

struct stdio_log *stderr_log;

int stdio_attach_stderr_log(struct stdio_log *log)
{
  stderr_log = log;

  return 0;
}

int stdio_log_init(struct stdio_log *log, size_t buffer_size)
{
  if (!(log->mutex = xSemaphoreCreateRecursiveMutex())) {
    LOG_ERROR("xSemaphoreCreateMutex");
    return -1;
  }

  if (!(log->buf = malloc(buffer_size))) {
    LOG_ERROR("malloc");
    return -1;
  }

  log->size = buffer_size;
  log->read = log->write = log->buf;

  LOG_BOOT_DEBUG("buf=%p size=%u", log->buf, log->size);

  return 0;
}

static size_t stdio_log_write_size(struct stdio_log *log)
{
  return log->buf + log->size - log->write;
}

static void stdio_log_write_len(struct stdio_log *log, size_t len)
{
  bool read_wrapped = false, write_wrapped = false;

  log->write += len;

  if (log->write_count > log->read_count && log->write >= log->read) {
    read_wrapped = true;
  }

  if (log->write >= log->buf + log->size) {
    write_wrapped = true;

    log->write_count++;
    log->write = log->buf;
  }

  if (read_wrapped) {
    log->read = log->write;
  }
  if (read_wrapped && write_wrapped) {
    log->read_count++;
  }
}

int stdio_log_new(struct stdio_log **logp, size_t buffer_size)
{
  struct stdio_log *log;
  int err;

  if (!(log = calloc(1, sizeof(*log)))) {
    LOG_ERROR("calloc");
    return -1;
  }

  if ((err = stdio_log_init(log, buffer_size))) {
    LOG_ERROR("stdio_log_init");
    goto error;
  }

  *logp = log;

  return 0;
error:
  free(log);

  return err;
}

size_t stdio_log_write(struct stdio_log *log, const void *data, size_t len)
{
  if (!xSemaphoreTakeRecursive(log->mutex, portMAX_DELAY)) {
    return 0;
  }

  size_t size = stdio_log_write_size(log);

  if (len > size) {
    len = size;
  }

  LOG_BOOT_DEBUG("write=%p count=%u size=%u len=%u", log->write, log->write_count, size, len);

  memcpy(log->write, data, len);

  stdio_log_write_len(log, len);

  xSemaphoreGiveRecursive(log->mutex);

  return len;
}

static size_t stdio_log_read_size(struct stdio_log *log)
{
  if (log->read == log->write && log->read_count >= log->write_count) {
    // special case: nothing written yet
    return 0;
  }  else if (log->read < log->write) {
    return log->write - log->read;
  } else {
    return log->buf + log->size - log->read;
  }
}

static void stdio_log_read_len(struct stdio_log *log, size_t len)
{
  log->read += len;

  if (log->read >= log->buf + log->size) {
    log->read_count++;
    log->read = log->buf;
  }
}

size_t stdio_log_read(struct stdio_log *log, void *data, size_t len)
{
  if (!xSemaphoreTakeRecursive(log->mutex, portMAX_DELAY)) {
    return 0;
  }

  size_t size = stdio_log_read_size(log);

  if (len > size) {
    len = size;
  }

  LOG_BOOT_DEBUG("read=%p count=%u size=%u len=%u", log->read, log->read_count, size, len);

  if (len) {
    memcpy(data, log->read, len);

    stdio_log_read_len(log, len);
  }

  xSemaphoreGiveRecursive(log->mutex);

  return len;
}

static void stdio_log_read_head(struct stdio_log *log)
{
  if (log->write_count > 0) {
    log->read = log->write;
    log->read_count = log->write_count - 1;
  } else {
    log->read = log->buf;
    log->read_count = 0;
  }
}

static void stdio_log_read_tail(struct stdio_log *log)
{
  log->read = log->write;
  log->read_count = log->write_count;
}

static off_t stdio_log_read_offset(struct stdio_log *log)
{
  return (log->read_count * log->size) + (log->read - log->buf);
}

off_t stdio_log_seek_set(struct stdio_log *log, off_t offset)
{
  off_t ret;

  if (offset) {
    // XXX: offset is not implemented
    return -1;
  }

  if (!xSemaphoreTakeRecursive(log->mutex, portMAX_DELAY)) {
    return 0;
  }
  stdio_log_read_head(log);

  ret = stdio_log_read_offset(log);

  xSemaphoreGiveRecursive(log->mutex);

  return ret;
}

off_t stdio_log_seek_cur(struct stdio_log *log, off_t offset)
{
  off_t ret;

  if (offset) {
    // XXX: offset is not implemented
    return -1;
  }

  if (!xSemaphoreTakeRecursive(log->mutex, portMAX_DELAY)) {
    return 0;
  }

  ret = stdio_log_read_offset(log);

  xSemaphoreGiveRecursive(log->mutex);

  return ret;
}

off_t stdio_log_seek_end(struct stdio_log *log, off_t offset)
{
  off_t ret;

  if (offset) {
    // XXX: offset is not implemented
    return -1;
  }

  if (!xSemaphoreTakeRecursive(log->mutex, portMAX_DELAY)) {
    return 0;
  }

  stdio_log_read_tail(log);

  ret = stdio_log_read_offset(log);

  xSemaphoreGiveRecursive(log->mutex);

  return ret;
}

#include <stdio_vfs.h>
#include "ets.h"
#include "log.h"
#include "uart.h"

#include <esp_err.h>
#include <esp_vfs.h>

#include <sys/unistd.h>
#include <sys/errno.h>

#define STDIO_VFS_FD_START 0
#define STDIO_VFS_FD_COUNT 3

ssize_t stdio_vfs_write(int fd, const void *data, size_t size)
{
    switch(fd) {
      case STDOUT_FILENO:
        if (stdio_uart) {
          return uart_write(stdio_uart, data, size);
        } else {
          ets_write(data, size);
          return size;
        }

      case STDERR_FILENO:
        if (stderr_log) {
          size = stdio_log_write(stderr_log, data, size);
        }

        if (!stdio_uart) {
          ets_write(data, size);
        } else if (uart_write_all(stdio_uart, data, size)) {
          errno = EIO;
          return -1;
        }

        return size;

      default:
        errno = EBADF;
        return -1;
    }
}

ssize_t stdio_vfs_read(int fd, void *data, size_t size)
{
    switch(fd) {
      case STDIN_FILENO:
        if (stdio_uart) {
          return uart_read(stdio_uart, data, size);
        } else {
          errno = ENODEV;
          return -1;
        }

      case STDERR_FILENO:
        if (stderr_log) {
          return stdio_log_read(stderr_log, data, size);
        } else {
          errno = ENODEV;
          return -1;
        }

      default:
        errno = EBADF;
        return -1;
    }
}

int stdio_vfs_fsync(int fd)
{
  switch(fd) {
    case STDOUT_FILENO:
    case STDERR_FILENO:
      if (stdio_uart) {
        return uart_flush_write(stdio_uart);
      } else {
        errno = ENODEV;
        return -1;
      }

    default:
      errno = EBADF;
      return -1;
  }

}

static const esp_vfs_t stdio_vfs = {
  .flags    = ESP_VFS_FLAG_DEFAULT,

  .write    = &stdio_vfs_write,
  .read     = &stdio_vfs_read,
  .fsync    = &stdio_vfs_fsync,
};

esp_err_t stdio_vfs_register()
{
  esp_err_t err;

  if ((err = esp_vfs_register_fd_range(&stdio_vfs, NULL, STDIO_VFS_FD_START, STDIO_VFS_FD_START + STDIO_VFS_FD_COUNT))) {
    return err;
  }

  return 0;
}

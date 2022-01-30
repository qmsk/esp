#include <stdio_vfs.h>
#include <stdio_fcntl.h>
#include "log.h"
#include "os.h"
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
          os_write(data, size);
          return size;
        }

      case STDERR_FILENO:
        if (stderr_log) {
          size = stdio_log_write(stderr_log, data, size);
        }

        if (!stdio_uart) {
          os_write(data, size);
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

off_t stdio_vfs_lseek_log(struct stdio_log *log, off_t size, int mode)
{
  off_t ret;

  switch(mode) {
    case SEEK_SET:
      if ((ret = stdio_log_seek_set(log, size)) < 0) {
        errno = EIO;
      }
      return ret;

    case SEEK_CUR:
      if ((ret = stdio_log_seek_cur(log, size)) < 0) {
        errno = EIO;
      }
      return ret;

    case SEEK_END:
      if ((ret = stdio_log_seek_end(log, size)) < 0) {
        errno = EIO;
      }
      return ret;

    default:
      errno = EINVAL;
      return -1;
  }
}

off_t stdio_vfs_lseek(int fd, off_t size, int mode)
{
  switch(fd) {
    case STDERR_FILENO:
      if (stderr_log) {
        return stdio_vfs_lseek_log(stderr_log, size, mode);
      } else {
        errno = ENODEV;
        return -1;
      }

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

int stdio_vfs_fcntl_uart(struct uart *uart, int cmd, int arg)
{
  switch(cmd) {
    case F_SET_READ_TIMEOUT:
      if (uart_set_read_timeout(uart, arg ? arg : portMAX_DELAY)) {
        errno = EIO;
        return -1;
      }

      return 0;

    default:
      errno = EINVAL;
      return -1;
  }
}

int stdio_vfs_fcntl(int fd, int cmd, int arg)
{
  switch(fd) {
    case STDIN_FILENO:
      if (stdio_uart) {
        return stdio_vfs_fcntl_uart(stdio_uart, cmd, arg);
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
  .lseek    = &stdio_vfs_lseek,
  .read     = &stdio_vfs_read,
  .fcntl    = &stdio_vfs_fcntl,
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

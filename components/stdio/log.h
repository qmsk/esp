#pragma once

#include <stdio_log.h>

#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include <stddef.h>

struct stdio_log {
    SemaphoreHandle_t *mutex;

    char *buf;
    size_t size;

    char *read, *write;
    unsigned read_count, write_count;
};

extern struct stdio_log *stderr_log;

/*
 * Return number of bytes copied.
 */
size_t stdio_log_write(struct stdio_log *log, const void *data, size_t len);

/*
 * Return number of bytes copied.
 */
size_t stdio_log_read(struct stdio_log *log, void *data, size_t len);

off_t stdio_log_seek_set(struct stdio_log *log, off_t offset);
off_t stdio_log_seek_cur(struct stdio_log *log, off_t offset);
off_t stdio_log_seek_end(struct stdio_log *log, off_t offset);

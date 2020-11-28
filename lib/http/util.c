#include "util.h"

#include "logging.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

const char *strdump (const char *str)
{
    static char buf[STRDUMP_MAX];

    const char *c = str;
    char *outc = buf;
    size_t len = sizeof(buf);
    int ret;

    while (*c) {
        if (*c == '\'') {
            ret = snprintf(outc, len, "\\'");
        } if (isprint((int) *c)) {
            ret = snprintf(outc, len, "%c", *c);
        } else if (*c == '\n') {
            ret = snprintf(outc, len, "\\n");
        } else if (*c == '\r') {
            ret = snprintf(outc, len, "\\r");
        } else if (*c == '\t') {
            ret = snprintf(outc, len, "\\t");
        } else {
            ret = snprintf(outc, len, "\\0x%02x", *c);
        }

        c++;

        if (ret < len) {
            outc += ret;
            len -= ret;
        } else {
            // truncated
            break;
        }
    }

    return buf;
}

int str_copy (char *buf, size_t size, const char *str)
{
    size_t len = strlen(str);

    if (len >= size)
        return 1;

    memcpy(buf, str, len);

    buf[len] = '\0';

    return 0;
}

int str_int (const char *str, int *intp)
{
    if (sscanf(str, "%d", intp) != 1) {
        LOG_WARN("invalid int: %s", str);
        return 1;
    }

    return 0;
}

int str_uint (const char *str, unsigned *uintp)
{
    if (sscanf(str, "%u", uintp) != 1) {
        LOG_WARN("invalid unsigned int: %s", str);
        return 1;
    }

    return 0;
}

const char * str_fmt (char *buf, size_t len, const char *fmt, ...)
{
    va_list args;
    int ret;

    va_start(args, fmt);
    ret = vsnprintf(buf, len, fmt, args);
    va_end(args);

    if (ret >= len) {
        LOG_WARN("truncated: %s -> %d", fmt, ret);
        return NULL;
    }

    return buf;
}

int timestamp_now (struct timeval *timestamp)
{
    if (gettimeofday(timestamp, NULL)) {
        LOG_ERROR("gettimeofday");
        return -1;
    }

    return 0;
}

int timestamp_from_timeout (struct timeval *timestamp, const struct timeval *timeout)
{
    if (gettimeofday(timestamp, NULL)) {
        LOG_ERROR("gettimeofday");
        return -1;
    }

    // set timeout in future
    // XXX: tv_usec overflow?
    timestamp->tv_sec += timeout->tv_sec;
    timestamp->tv_usec += timeout->tv_usec;

    return 0;
}

int timeout_from_timestamp (struct timeval *timeout, const struct timeval *timestamp)
{
    if (gettimeofday(timeout, NULL)) {
        LOG_WARN("gettimeofday");
        return -1;
    }

    if (timestamp->tv_sec >= timeout->tv_sec && timestamp->tv_usec >= timeout->tv_usec) {
        timeout->tv_sec = timestamp->tv_sec - timeout->tv_sec;
        timeout->tv_usec = timestamp->tv_usec - timeout->tv_usec;

    } else if (timestamp->tv_sec > timeout->tv_sec && timestamp->tv_usec < timeout->tv_usec) {
        timeout->tv_sec = timestamp->tv_sec - timeout->tv_sec - 1;
        timeout->tv_usec = 1000000 + timestamp->tv_usec - timeout->tv_usec;

    } else {
        LOG_WARN("timestamp in past: %ld:%ld < %ld:%ld",
                timeout->tv_sec, timeout->tv_usec,
                timestamp->tv_sec, timestamp->tv_usec
        );
        timeout->tv_sec = 0;
        timeout->tv_usec = 0;

        return 1;
    }

    LOG_DEBUG("%ld:%ld <- %ld:%ld", timestamp->tv_sec, timestamp->tv_usec, timeout->tv_sec, timeout->tv_usec);

    return 0;
}

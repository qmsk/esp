#pragma once

#include <json.h>

/* Raw char */
int json_writec(struct json_writer *w, enum json_token token);

/* Raw string */
int json_writef(struct json_writer *w, enum json_token token, const char *fmt, ...);
int json_writev(struct json_writer *w, enum json_token token, const char *fmt, va_list args);

/* Quoted string */
int json_writeq(struct json_writer *w,  enum json_token token, const char *string, int size);

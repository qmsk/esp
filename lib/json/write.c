#include "json.h"
#include "logging.h"

#include <stdarg.h>
#include <stdio.h>

int json_writer_init(struct json_writer *w, FILE *f)
{
  w->file = f;
  w->stack[0] = JSON;
  w->stackp = w->stack;

  return 0;
}

static int json_write_sep(struct json_writer *w, enum json_token token)
{
  if (fputc(token, w->file) == EOF) {
    LOG_ERROR("fputc");
    return -1;
  }

  *w->stackp = token;

  return 0;
}

static int json_write_pre(struct json_writer *w)
{
  if (*w->stackp == JSON || *w->stackp == JSON_OBJECT_MEMBER) {
    return 0;
  }

  if (w->stackp <= w->stack) {
    LOG_ERROR("top-level element already written");
    return -2;
  }

  if (json_write_sep(w, JSON_COMMA)) {
    return -1;
  }

  return 0;
}

static int json_write(struct json_writer *w, enum json_token token)
{
  if (json_write_pre(w)) {
    return -1;
  }

  if (fputc(token, w->file) == EOF) {
    LOG_ERROR("fputc");
    return -1;
  }

  *w->stackp = token;

  return 0;
}

static int json_writef(struct json_writer *w, enum json_token token, const char *fmt, ...)
{
  va_list args;
  int ret;

  if (json_write_pre(w)) {
    return -1;
  }

  va_start(args, fmt);
  ret = vfprintf(w->file, fmt, args);
  va_end(args);

  if (ret < 0) {
    return -1;
  }

  *w->stackp = token;

  return 0;
}

static int json_write_push(struct json_writer *w, enum json_token token)
{
  if (json_write_pre(w)) {
    return -1;
  }

  if (fputc(token, w->file) == EOF) {
    LOG_ERROR("fputc");
    return -1;
  }

  *w->stackp = token;

  if (w->stackp >= w->stack + JSON_STACK_SIZE) {
    return -2;
  } else {
    *++w->stackp = JSON;
  }

  return 0;
}

static int json_write_pop(struct json_writer *w, enum json_token token, enum json_token context)
{
  if (w->stackp <= w->stack) {
    return -2;
  } else {
    --w->stackp;
  }

  if (*w->stackp != context) {
    return -2;
  }

  if (fputc(token, w->file) == EOF) {
    LOG_ERROR("fputc");
    return -1;
  }

  *w->stackp = token;

  return 0;
}


int json_write_string(struct json_writer *w, const char *string)
{
  if (json_write(w, JSON_STRING)) {
    return -1;
  }

  for (const char *c = string; *c; c++) {
    switch(*c) {
      case '\b':
        fputs("\\b", w->file);
        break;

      case '\f':
        fputs("\\f", w->file);
        break;

      case '\n':
        fputs("\\n", w->file);
        break;

      case '\r':
        fputs("\\r", w->file);
        break;

      case '\t':
        fputs("\\t", w->file);
        break;

      case '"':
        fputs("\\\"", w->file);
        break;

      case '\\':
        fputs("\\\\", w->file);
        break;

      default:
        fputc(*c, w->file);
        break;
    }
  }

  if (fputc('"', w->file) == EOF) {
    LOG_ERROR("fputc");
    return -1;
  }

  return 0;
}

int json_write_int(struct json_writer *w, int value)
{
  return json_writef(w, JSON_NUMBER, "%d", value);
}

int json_write_uint(struct json_writer *w, unsigned value)
{
  return json_writef(w, JSON_NUMBER, "%u", value);
}

int json_open_object(struct json_writer *w)
{
  return json_write_push(w, JSON_OBJECT);
}

int json_open_object_member(struct json_writer *w, const char *name)
{
  return json_write_string(w, name) || json_write_sep(w, JSON_OBJECT_MEMBER);
}

int json_close_object(struct json_writer *w)
{
  return json_write_pop(w, JSON_OBJECT_END, JSON_OBJECT);
}

int json_open_array(struct json_writer *w)
{
  return json_write_push(w, JSON_ARRAY);
}

int json_close_array(struct json_writer *w)
{
  return json_write_pop(w, JSON_ARRAY_END, JSON_ARRAY);
}

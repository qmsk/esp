#include <json.h>
#include <logging.h>

#include "write.h"

#include <stdarg.h>
#include <stdio.h>

int json_writer_init(struct json_writer *w, FILE *f)
{
  w->file = f;
  w->stack[0] = JSON;
  w->stackp = w->stack;

  return 0;
}

static int json_push_token(struct json_writer *w, enum json_token token)
{
  if (w->stackp >= w->stack + JSON_STACK_SIZE) {
    LOG_ERROR("stack overflow");
    return -2;
  }

  w->stackp++;
  *w->stackp = token;

  return 0;
}

static int json_set_token(struct json_writer *w, enum json_token token)
{
  *w->stackp = token;

  return 0;
}

static int json_pop_token(struct json_writer *w, enum json_token token, enum json_token context)
{
  if (w->stackp <= w->stack) {
    LOG_ERROR("stack underflow");
    return -2;
  }

  w->stackp--;

  if (*w->stackp != context) {
    LOG_ERROR("expected context %c for token %c, actual context %c", context, token, *w->stackp);
    return -2;
  }

  *w->stackp = token;

  return 0;
}

static int json_start_write(struct json_writer *w, enum json_token token)
{
  enum json_token state = *w->stackp;
  int depth = w->stackp - w->stack;
  int err;

  LOG_DEBUG("depth %d @ state %c : token %c", depth, state, token);

  if (state == JSON) {
    // initial element
    err = json_set_token(w, token);

  } else if (state == JSON_ARRAY || state == JSON_OBJECT) {
    // enter object/array
    err = json_push_token(w, token);

  } else if (state == JSON_OBJECT_MEMBER) {
    // key separator
    if (fputs(": ", w->file) == EOF) {
      LOG_ERROR("fputs");
      return -1;
    }

    err = json_set_token(w, token);

  } else if (depth <= 0) {
    LOG_ERROR("top-level element already written");
    return -2;

  } else if (token == JSON_OBJECT_END || token == JSON_ARRAY_END) {
    // no separator before end token
    err = json_set_token(w, token);

  } else {
    // comma separator
    if (fputs(", ", w->file) == EOF) {
      LOG_ERROR("fputs");
      return -1;
    }

    err = json_set_token(w, token);
  }

  // exit
  if (err) {
    return err;

  } else if (token == JSON_OBJECT_END) {
    // exit object
    return json_pop_token(w, token, JSON_OBJECT);

  } else if (token == JSON_ARRAY_END) {
    // exit array
    return json_pop_token(w, token, JSON_ARRAY);

  } else {
    return 0;
  }
}

int json_writec(struct json_writer *w, enum json_token token)
{
  if (json_start_write(w, token)) {
    return -1;
  }

  if (fputc(token, w->file) == EOF) {
    LOG_ERROR("fputc");
    return -1;
  }

  return 0;
}

int json_writef(struct json_writer *w, enum json_token token, const char *fmt, ...)
{
  va_list args;
  int ret;

  if (json_start_write(w, token)) {
    return -1;
  }

  va_start(args, fmt);
  ret = vfprintf(w->file, fmt, args);
  va_end(args);

  if (ret < 0) {
    return -1;
  }

  return 0;
}

int json_writev(struct json_writer *w, enum json_token token, const char *fmt, va_list args)
{
  int ret;

  if (json_start_write(w, token)) {
    return -1;
  }

  ret = vfprintf(w->file, fmt, args);

  if (ret < 0) {
    return -1;
  }

  return 0;
}

int json_writeq(struct json_writer *w,  enum json_token token, const char *string, int size)
{
  const char *end = (size >= 0) ? string + size : NULL;

  if (json_start_write(w, token)) {
    return -1;
  }

  if (fputc('"', w->file) == EOF) {
    LOG_ERROR("fputc");
    return -1;
  }

  for (const char *c = string; (end ? c < end : true) && *c; c++) {
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

int json_write_raw(struct json_writer *w, const char *fmt, ...)
{
  va_list args;
  int ret;

  va_start(args, fmt);
  ret = json_writev(w, JSON_RAW, fmt, args);
  va_end(args);

  return ret;
}

int json_write_string(struct json_writer *w, const char *value)
{
  if (value) {
    return json_writeq(w, JSON_STRING, value, -1);
  } else {
    return json_writef(w, JSON_NULL, "null");
  }
}

int json_write_nstring(struct json_writer *w, const char *value, size_t size)
{
  if (value) {
    return json_writeq(w, JSON_STRING, value, size);
  } else {
    return json_writef(w, JSON_NULL, "null");
  }
}

int json_write_int(struct json_writer *w, int value)
{
  return json_writef(w, JSON_NUMBER, "%d", value);
}

int json_write_uint(struct json_writer *w, unsigned value)
{
  return json_writef(w, JSON_NUMBER, "%u", value);
}

int json_write_bool(struct json_writer *w, bool value)
{
  if (value) {
    return json_writef(w, JSON_TRUE, "true");
  } else {
    return json_writef(w, JSON_FALSE, "false");
  }
}

int json_write_null(struct json_writer *w)
{
  return json_writef(w, JSON_NULL, "null");
}

int json_open_object(struct json_writer *w)
{
  return json_writec(w, JSON_OBJECT);
}

int json_open_object_member(struct json_writer *w, const char *name)
{
  return json_writeq(w, JSON_OBJECT_MEMBER, name, -1);
}

int json_close_object(struct json_writer *w)
{
  return json_writec(w, JSON_OBJECT_END);
}

int json_open_array(struct json_writer *w)
{
  return json_writec(w, JSON_ARRAY);
}

int json_close_array(struct json_writer *w)
{
  return json_writec(w, JSON_ARRAY_END);
}

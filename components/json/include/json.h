#pragma once

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#define JSON_STACK_SIZE 8

enum json_token {
  JSON                = '\0',
  JSON_COMMA          = ',',

  JSON_RAW            = '?',
  JSON_STRING         = '"',
  JSON_NUMBER         = '0',
  JSON_TRUE           = 't',
  JSON_FALSE          = 'f',
  JSON_NULL           = 'n',

  JSON_OBJECT         = '{',
  JSON_OBJECT_MEMBER  = ':',
  JSON_OBJECT_END     = '}',

  JSON_ARRAY          = '[',
  JSON_ARRAY_END      = ']',

};

struct json_writer {
  FILE *file;
  enum json_token stack[JSON_STACK_SIZE], *stackp;
};

int json_writer_init(struct json_writer *w, FILE *f);

int json_write_raw(struct json_writer *w, const char *fmt, ...);
int json_write_string(struct json_writer *w, const char *value);
int json_write_nstring(struct json_writer *w, const char *value, size_t size);
int json_write_float(struct json_writer *w, float value);
int json_write_int(struct json_writer *w, int value);
int json_write_uint(struct json_writer *w, unsigned value);
int json_write_bool(struct json_writer *w, bool value);
int json_write_null(struct json_writer *w);

int json_open_object(struct json_writer *w);
int json_open_object_member(struct json_writer *w, const char *name);
int json_close_object(struct json_writer *w);

int json_open_array(struct json_writer *w);
int json_close_array(struct json_writer *w);

#define JSON_WRITE_OBJECT(w, write_members) (json_open_object(w) || (write_members) || json_close_object(w))
#define JSON_WRITE_ARRAY(w, write_members) (json_open_array(w) || (write_members) || json_close_array(w))

#define JSON_WRITE_MEMBER(w, name, write_member) (json_open_object_member((w), (name)) || (write_member))
#define JSON_WRITE_MEMBER_RAW(w, name, ...) (json_open_object_member((w), (name)) || json_write_raw((w), __VA_ARGS__))
#define JSON_WRITE_MEMBER_STRING(w, name, value) (json_open_object_member((w), (name)) || json_write_string((w), (value)))
#define JSON_WRITE_MEMBER_NSTRING(w, name, value) (json_open_object_member((w), (name)) || json_write_nstring((w), (value), sizeof(value)))
#define JSON_WRITE_MEMBER_FLOAT(w, name, value) (json_open_object_member((w), (name)) || json_write_float((w), (value)))
#define JSON_WRITE_MEMBER_INT(w, name, value) (json_open_object_member((w), (name)) || json_write_int((w), (value)))
#define JSON_WRITE_MEMBER_UINT(w, name, value) (json_open_object_member((w), (name)) || json_write_uint((w), (value)))
#define JSON_WRITE_MEMBER_BOOL(w, name, value) (json_open_object_member((w), (name)) || json_write_bool((w), (value)))
#define JSON_WRITE_MEMBER_NULL(w, name) (json_open_object_member((w), (name)) || json_write_null((w)))
#define JSON_WRITE_MEMBER_OBJECT(w, name, write_object) (json_open_object_member((w), (name)) || json_open_object(w) || (write_object) || json_close_object(w))
#define JSON_WRITE_MEMBER_ARRAY(w, name, write_array) (json_open_object_member((w), (name)) || json_open_array(w) || (write_array) || json_close_array(w))

/* Optional 64-bit support */
#if !CONFIG_NEWLIB_NANO_FORMAT

int json_write_int64(struct json_writer *w, int64_t value);
int json_write_uint64(struct json_writer *w, uint64_t value);

#define JSON_WRITE_MEMBER_INT64(w, name, value) (json_open_object_member((w), (name)) || json_write_int64((w), (value)))
#define JSON_WRITE_MEMBER_UINT64(w, name, value) (json_open_object_member((w), (name)) || json_write_uint((w), (value)))

#endif

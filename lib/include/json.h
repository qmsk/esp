#ifndef __LIB_JSON_H__
#define __LIB_JSON_H__

#include <stdio.h>

#define JSON_STACK_SIZE 8

enum json_token {
  JSON                = '\0',
  JSON_COMMA          = ',',

  JSON_STRING         = '"',
  JSON_NUMBER         = '0',

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

int json_write_string(struct json_writer *w, const char *string);
int json_write_int(struct json_writer *w, int value);
int json_write_uint(struct json_writer *w, unsigned value);

int json_open_object(struct json_writer *w);
int json_open_object_member(struct json_writer *w, const char *name);
int json_close_object(struct json_writer *w);

int json_open_array(struct json_writer *w);
int json_close_array(struct json_writer *w);

#define JSON_WRITE_OBJECT(w, write_members) (json_open_object(w) || (write_members) || json_close_object(w))
#define JSON_WRITE_OBJECT_STRING(w, name, value) (json_open_object_member((w), (name)) || json_write_string((w), (value)))
#define JSON_WRITE_OBJECT_OBJECT(w, name, write_object) (json_open_object_member((w), (name)) || json_open_object(w) || (write_object) || json_close_object(w))
#define JSON_WRITE_OBJECT_ARRAY(w, name, write_array) (json_open_object_member((w), (name)) || json_open_array(w) || (write_array) || json_close_array(w))

#endif

#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include <c_types.h>

enum config_type {
  CONFIG_TYPE_NULL,
  CONFIG_TYPE_UINT16,
  CONFIG_TYPE_STRING,
};

struct config_tab {
  enum config_type type;
  const char *name;
  size_t size;
  bool readonly;
  union {
    uint16_t *uint16;
    char *string;
  } value;
};

#endif

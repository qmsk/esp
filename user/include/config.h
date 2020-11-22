#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include <c_types.h>

#define CONFIG_VALUE_SIZE 256

enum config_type {
  CONFIG_TYPE_NULL,

  CONFIG_TYPE_UINT16,
  CONFIG_TYPE_STRING,
};

struct configtab {
  enum config_type type;
  const char *name;

  size_t size;
  bool readonly;
  bool secret;

  union {
    uint16_t *uint16;
    char *string;
  } value;
};

struct configmod {
  const char *name;

  const struct configtab *table;
};

struct config {
  const struct configmod *modules;
};

#endif

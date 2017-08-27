#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

#include "user_config.h"
#include <cmd.h>

extern struct user_config user_config;

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

int init_config(struct user_config *config);

extern const struct cmdtab config_cmdtab;


#endif

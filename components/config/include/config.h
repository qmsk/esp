#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "cmd.h"

#define CONFIG_FILENAME 64
#define CONFIG_LINE 512
#define CONFIG_NAME_SIZE 64
#define CONFIG_VALUE_SIZE 256

enum config_type {
  CONFIG_TYPE_NULL,

  CONFIG_TYPE_UINT16,
  CONFIG_TYPE_STRING,
  CONFIG_TYPE_BOOL,
  CONFIG_TYPE_ENUM,
};

struct config_enum {
  const char *name;

  int value;
};

struct configtab {
  enum config_type type;
  const char *name;

  size_t size;
  const struct config_enum *enum_values;
  bool readonly;
  bool secret;

  union {
    uint16_t *uint16;
    char *string;
    bool *boolean;
    int *enum_value;
  } value;
};

struct configmod {
  const char *name;

  const struct configtab *table;
};

struct config {
  const char *filename;

  const struct configmod *modules;
};

int config_enum_lookup(const struct config_enum *e, const char *name, const struct config_enum **enump);
int config_enum_find_by_value(const struct config_enum *e, int value, const struct config_enum **enump);

int configmod_lookup(const struct configmod *mod, const char *name, const struct configmod **modp);
int configtab_lookup(const struct configtab *tab, const char *name, const struct configtab **tabp);
int config_lookup(const struct config *config, const char *module, const char *name, const struct configmod **modp, const struct configtab **tabp);

int config_set(const struct configmod *mod, const struct configtab *tab, const char *value);
int config_get(const struct configmod *mod, const struct configtab *tab, char *buf, size_t size);
int config_print(const struct configmod *mod, const struct configtab *tab, FILE *file);

int config_read(struct config *config, FILE *file);
int config_write(struct config *config, FILE *file);

int config_load(struct config *config);
int config_save(struct config *config);
int config_clear(struct config *config);

extern const struct cmd config_commands[];

#endif

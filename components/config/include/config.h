#pragma once

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#define CONFIG_BOOT_FILE "boot.ini"

#define CONFIG_PATH_SIZE 64
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
  const char *description;

  /* Migrate from old name */
  const char *alias;

  /* Support multiple values */
  unsigned *count, size;

  bool readonly;
  bool secret;

  union {
    struct {
      uint16_t *value;

      /* inclusive, default 0 -> unlimited */
      uint16_t max;

      uint16_t default_value;
    } uint16_type;

    struct {
      char *value;
      size_t size;
      const char *default_value;
    } string_type;

    struct {
      bool *value;
      bool default_value;
    } bool_type;

    struct {
      int *value;
      const struct config_enum *values;
      int default_value;
    } enum_type;
  };
};

struct configmod {
  const char *name;
  const char *description;

  union {
    const struct configtab *table;
    const struct configtab **tables;  // table_count
  };
  int tables_count;

  /* Migrate from old name */
  const char *alias;
};

struct config {
  const char *path;

  const struct configmod *modules;
};

int config_enum_lookup(const struct config_enum *e, const char *name, const struct config_enum **enump);
int config_enum_find_by_value(const struct config_enum *e, int value, const struct config_enum **enump);

/* Return name for enum value, or NULL */
const char *config_enum_to_string(const struct config_enum *e, int value);

/* Return value for enum name, or -1 */
int config_enum_to_value(const struct config_enum *e, const char *name);

int configmod_lookup(const struct configmod *modules, const char *name, const struct configmod **modp, const struct configtab **tablep);
int configtab_lookup(const struct configtab *table, const char *name, const struct configtab **tabp);
int config_lookup(const struct config *config, const char *module, const char *name, const struct configmod **modp, const struct configtab **tabp);

/* Return count of values for use with index. This is typically typically 1, if not multi-valued */
static inline int configtab_count(const struct configtab *tab)
{
  if (tab->count) {
    return *tab->count;
  } else {
    return 1;
  }
}

/*
 * Set default values.
 *
 * Multi-valued configtabs are cleared.
 */
int config_init(struct config *config);

/* Set empty value, or reset count to 0 if multi-valued */
int config_clear(const struct configmod *mod, const struct configtab *tab);

/* Set value, or add new value for multi-valued configtab */
int config_set(const struct configmod *mod, const struct configtab *tab, const char *value);

/* Get value at index (typically 0, if not multi-valued) */
int config_get(const struct configmod *mod, const struct configtab *tab, unsigned index, char *buf, size_t size);

/* Print value at index (typically 0, if not multi-valued) to file */
int config_print(const struct configmod *mod, const struct configtab *tab, unsigned index, FILE *file);

/*
 * Set config from file contents.
 *
 * The config must be empty (`config_init()`), or values will be duplicated!
 */
int config_read(struct config *config, FILE *file);

/*
 * Write config to file.
 */
int config_write(struct config *config, FILE *file);

/*
 * Load config from file.
 */
int config_load(struct config *config, const char *filename);

/*
 * Save config to file.
 */
int config_save(struct config *config, const char *filename);

/*
 * Remove config file.
 *
 * Returns <0 on error, 0 if reset, >0 if no file to remove.
 */
int config_delete(struct config *config, const char *filename);

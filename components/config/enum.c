#include <config.h>

#include <string.h>

int config_enum_lookup(const struct config_enum *e, const char *name, const struct config_enum **enump)
{
  for (; e->name; e++) {
    if (strcmp(e->name, name) == 0) {
      *enump = e;
      return 0;
    }
  }

  return 1;
}

int config_enum_find_by_value(const struct config_enum *e, int value, const struct config_enum **enump)
{
  for (; e->name; e++) {
    if (e->value == value) {
      *enump = e;
      return 0;
    }
  }

  return 1;
}

const char *config_enum_to_string(const struct config_enum *e, int value)
{
  for (; e->name; e++) {
    if (e->value == value) {
      return e->name;
    }
  }

  return NULL;
}

int config_enum_to_value(const struct config_enum *e, const char *name)
{
  for (; e->name; e++) {
    if (strcmp(e->name, name) == 0) {
      return e->value;
    }
  }

  return -1;
}

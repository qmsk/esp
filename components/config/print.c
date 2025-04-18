#include <config.h>

#include <logging.h>

#include <stdio.h>

static int config_print_enum(const struct configmod *mod, const struct configtab *tab, unsigned index, FILE *file)
{
  const struct config_enum *e;

  if (config_enum_find_by_value(tab->enum_type.values, tab->enum_type.value[index], &e)) {
    LOG_ERROR("%s.%s: unknown value: %#x", mod->name, tab->name, tab->enum_type.value[index]);
    return -1;
  }

  LOG_DEBUG("%s.%s: value=%#x enum=%s", mod->name, tab->name, tab->enum_type.value[index], e->name);

  if (fprintf(file, "%s", e->name) < 0) {
    return -1;
  }

  return 0;
}

int config_print(const struct configmod *mod, const struct configtab *tab, unsigned index, FILE *file)
{
  if (tab->migrated) {
    LOG_ERROR("type=%u name=%s migrated", tab->type, tab->name);
    return -1;
  }

  if (index >= configtab_count(tab)) {
    LOG_ERROR("invalid index=%u for count=%u", index, configtab_count(tab));
  }

  LOG_DEBUG("type=%u name=%s index=%u", tab->type, tab->name, index);

  switch (tab->type) {
  case CONFIG_TYPE_NULL:
    break;

  case CONFIG_TYPE_UINT16:
    if (fprintf(file, "%u", tab->uint16_type.value[index]) < 0) {
      return -1;
    }
    break;

  case CONFIG_TYPE_STRING:
    if (fprintf(file, "%s", &tab->string_type.value[index * tab->string_type.size]) < 0) {
      return -1;
    }
    break;

  case CONFIG_TYPE_BOOL:
    if (fprintf(file, "%s", tab->bool_type.value[index] ? "true" : "false") < 0) {
      return -1;
    }
    break;

  case CONFIG_TYPE_ENUM:
    return config_print_enum(mod, tab, index, file);

  case CONFIG_TYPE_FILE:
    if (fprintf(file, "%s", &tab->file_type.value[index * tab->file_type.size]) < 0) {
      return -1;
    }
    break;

  default:
    LOG_ERROR("invalid type=%d", tab->type);
    return -1;
  }

  return 0;
}

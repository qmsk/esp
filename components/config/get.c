#include <config.h>

#include <logging.h>

#include <stdio.h>

int config_get_enum(const struct configmod *mod, const struct configtab *tab, unsigned index, char *buf, size_t size)
{
  const struct config_enum *e;

  if (config_enum_find_by_value(tab->enum_type.values, tab->enum_type.value[index], &e)) {
    LOG_ERROR("%s.%s: unknown value: %#x", mod->name, tab->name, tab->enum_type.value[index]);
    return -1;
  }

  if (snprintf(buf, size, "%s", e->name) >= size) {
    return -1;
  }

  return 0;
}

int config_get(const struct configmod *mod, const struct configtab *tab, unsigned index, char *buf, size_t size)
{
  if (index >= configtab_count(tab)) {
    LOG_ERROR("invalid index=%u for count=%u", index, configtab_count(tab));
  }

  LOG_DEBUG("type=%u name=%s index=%u", tab->type, tab->name, index);

  switch(tab->type) {
    case CONFIG_TYPE_NULL:
      break;

    case CONFIG_TYPE_UINT16:
      if (snprintf(buf, size, "%u", tab->uint16_type.value[index]) >= size) {
        return -1;
      } else {
        break;
      }

    case CONFIG_TYPE_STRING:
      if (snprintf(buf, size, "%s", &tab->string_type.value[index * tab->string_type.size]) >= size) {
        return -1;
      } else {
        break;
      }

    case CONFIG_TYPE_BOOL:
      if (snprintf(buf, size, "%s", tab->bool_type.value[index] ? "true" : "false") >= size) {
        return -1;
      } else {
        break;
      }

    case CONFIG_TYPE_ENUM:
      return config_get_enum(mod, tab, index, buf, size);

    case CONFIG_TYPE_FILE:
      if (snprintf(buf, size, "%s", &tab->file_type.value[index * tab->file_type.size]) >= size) {
        return -1;
      } else {
        break;
      }

    default:
      LOG_ERROR("invalid type=%d", tab->type);
      return -1;
  }

  return 0;
}

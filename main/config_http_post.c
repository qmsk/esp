#include "config.h"
#include "http_routes.h"
#include "http_handlers.h"

#include <logging.h>
#include <json.h>


static int config_api_parse_name (char *key, const char **modulep, const char **namep)
{
  *modulep = *namep = NULL;

  for (char *c = key; *c; c++) {
    if (*c == '[') {
      *modulep = c+1;
    } else if (*c == ']' ) {
      *c = '\0';
      *namep = c+1;
    }
  }

  if (!*modulep) {
    LOG_WARN("no [module]... given");
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  if (!*namep) {
    LOG_WARN("no [...]name given");
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  return 0;
}

static int config_api_set (struct config *config, char *key, char *value)
{
  const char *module, *name;
  struct config_path path;
  int err;

  if ((err = config_api_parse_name(key, &module, &name))) {
    LOG_WARN("config_api_parse_name: %s", key);
    return err;
  }

  if ((err = config_lookup(config, module, name, &path))) {
    LOG_WARN("config_lookup: module=%s name=%s", module, name);
    return HTTP_UNPROCESSABLE_ENTITY;
  }

  if (value && *value) {
    LOG_INFO("module=%s index=%u name=%s: set value=%s", path.mod->name, path.index, path.tab->name, value);

    if ((err = config_set(path, value))) {
      LOG_WARN("config_set: module=%s index=%u name=%s: value=%s", path.mod->name, path.index, path.tab->name, value);
      return HTTP_UNPROCESSABLE_ENTITY;
    }
  } else {
    LOG_INFO("module=%s index=%u name=%s: clear", path.mod->name, path.index, path.tab->name);

    if ((err = config_clear(path))) {
      LOG_WARN("config_clear: module=%s index=%u name=%s", path.mod->name, path.index, path.tab->name);
      return HTTP_UNPROCESSABLE_ENTITY;
    }
  }

  return 0;
}

/* POST /api/config application/x-www-form-urlencoded */
int config_api_post_form(struct http_request *request, struct http_response *response, void *ctx)
{
    char *key, *value;
    int err;

    while (!(err = http_request_form(request, &key, &value))) {
      if ((err = config_api_set(&config, key, value))) {
        LOG_WARN("config_api_set %s", key);
        return err;
      }
    }

    if (err < 0) {
      LOG_ERROR("http_request_form");
      return err;
    }

    LOG_INFO("config save %s", CONFIG_BOOT_FILE);

    if ((err = config_save(&config, CONFIG_BOOT_FILE))) {
      LOG_ERROR("config_save");
      return err;
    }

    return HTTP_NO_CONTENT;
}

int config_api_post(struct http_request *request, struct http_response *response, void *ctx)
{
  const struct http_request_headers *headers;
  int err;

  if ((err = http_request_headers(request, &headers))) {
    LOG_WARN("http_request_headers");
    return err;
  }

  switch (headers->content_type) {
    case HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED:
      return config_api_post_form(request, response, ctx);

    default:
      LOG_WARN("Unkonwn Content-Type");

      return HTTP_UNSUPPORTED_MEDIA_TYPE;
  }
}

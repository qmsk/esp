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
    return -1;
  }

  if (!*namep) {
    return -1;
  }

  return 0;
}

static int config_api_set (struct config *config, char *key, char *value, struct http_response *response)
{
  const char *module, *name;
  struct config_path path;
  int err;

  if ((err = config_api_parse_name(key, &module, &name))) {
    return http_response_error(response, HTTP_UNPROCESSABLE_ENTITY, NULL, "Invalid config key: %s\n", key);
  }

  if ((err = config_lookup(config, module, name, &path))) {
    return http_response_error(response, HTTP_UNPROCESSABLE_ENTITY, NULL, "No matching config found: [%s]%s\n", module, name);
  }

  if (value && *value) {
    LOG_INFO("module=%s index=%u name=%s: set value=%s", path.mod->name, path.index, path.tab->name, value);

    if ((err = config_set(path, value))) {
      return http_response_error(response, HTTP_UNPROCESSABLE_ENTITY, NULL, "Invalid config value: [%s]%s = %s\n", module, name, value);
    }
  } else {
    LOG_INFO("module=%s index=%u name=%s: clear", path.mod->name, path.index, path.tab->name);

    if ((err = config_clear(path))) {
      return http_response_error(response, HTTP_UNPROCESSABLE_ENTITY, NULL, "Invalid config clear: [%s]%s\n", module, name);
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
      if ((err = config_api_set(&config, key, value, response))) {
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
    LOG_ERROR("http_request_headers");
    return err;
  }

  switch (headers->content_type) {
    case HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED:
      return config_api_post_form(request, response, ctx);

    default:
      return http_response_error(response, HTTP_UNSUPPORTED_MEDIA_TYPE, NULL, "Unsupported Content-Type\n");
  }
}

#include "mdns.h"
#include "system.h"

#include <logging.h>

#include <esp_err.h>
#include <mdns.h>

#if CONFIG_ENABLE_MDNS
int init_mdns()
{
  int err;
  const char *hostname;

  if ((err = get_system_hostname(&hostname)) < 0){
    LOG_ERROR("get_mdns_hostname");
    return err;
  } else if (err) {
    LOG_WARN("no hostname available, skip");
    return 0;
  } else {
    LOG_INFO("hostname=%s", hostname);
  }

  if ((err = mdns_init())) {
    LOG_ERROR("mdns_init: %s", esp_err_to_name(err));
    return -1;
  }

  if ((err = mdns_hostname_set(hostname))) {
    LOG_ERROR("mdns_hostname_set: %s", esp_err_to_name(err));
    return -1;
  }

  return 0;
}
#else
int init_mdns()
{
  LOG_INFO("disabled");
}
#endif

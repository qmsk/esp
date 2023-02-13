#pragma once

#include <sdkconfig.h>
#include <cmd.h>

#if CONFIG_SDCARD_ENABLED
  int init_sdcard();

  extern const struct cmdtab sdcard_cmdtab;
#endif

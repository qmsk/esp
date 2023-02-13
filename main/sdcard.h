#pragma once

#include <sdkconfig.h>
#include <cmd.h>

#if CONFIG_SDCARD_ENABLED
  int init_sdcard();
  int start_sdcard();
  int stop_sdcard();

  extern const struct cmdtab sdcard_cmdtab;
#endif

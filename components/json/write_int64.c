#include <json.h>
#include "write.h"

#if !CONFIG_NEWLIB_NANO_FORMAT

#include <inttypes.h>

int json_write_int64(struct json_writer *w, int64_t value)
{
  return json_writef(w, JSON_NUMBER, "%" PRId64, value);
}

int json_write_uint64(struct json_writer *w, uint64_t value)
{
  return json_writef(w, JSON_NUMBER, "%" PRIu64, value);
}

#endif

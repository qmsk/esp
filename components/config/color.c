#include <config.h>

#include <logging.h>

#include <string.h>

int config_color_parse(struct config_color *color, const char *value)
{
  int r = 0, g = 0, b = 0, a = 0;

  switch (strlen(value)) {
    case 3:
      a = 0xff;

      if (sscanf(value, "%1x%1x%1x", &r, &g, &b) < 3) {
        return -1;
      }

      r = (r << 4) | r;
      g = (g << 4) | g;
      b = (b << 4) | b;

      break;

    case 6:
      a = 0xff;
      
      if (sscanf(value, "%2x%2x%2x", &r, &g, &b) < 3) {
        return -1;
      }

      break;

    case 8:
      if (sscanf(value, "%2x%2x%2x%2x", &r, &g, &b, &a) < 4) {
        return -1;
      }

      break;
    
    default:
      return -1;
  }

  *color = (struct config_color) { r, g, b, a };

  return 0;
}

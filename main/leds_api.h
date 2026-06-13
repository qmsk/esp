#pragma once

#include <leds.h>
#include <json.h>

int leds_api_write_color(struct json_writer *w, struct leds_color c, enum leds_parameter_type parameter_type);

int leds_api_color_parse(struct leds_color *color, enum leds_parameter_type parameter_type, const char *value);

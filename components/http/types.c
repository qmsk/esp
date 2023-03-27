#include "http/http_types.h"

#include <errno.h>
#include <string.h>
#include <strings.h>
#include <time.h>

#define HTTP_DATE_FORMAT "%a, %d %b %Y %H:%M:%S GMT"

const char *http_version_str (enum http_version version)
{
  switch (version) {
    case HTTP_10:   return "HTTP/1.0";
    case HTTP_11:   return "HTTP/1.1";

    default:        return "HTTP";
  }
}

const char *http_status_str (enum http_status status)
{
    switch (status) {
        case 200:   return "OK";
        case 201:   return "Created";
        case 204:   return "No Content";

        case 301:   return "Found";

        case 400:   return "Bad Request";
        case 401:   return "Unauthorized";
        case 403:   return "Forbidden";
        case 404:   return "Not Found";
        case 405:   return "Method Not Allowed";
        case 409:   return "Conflict";
        case 411:   return "Length Required";
        case 413:   return "Request Entity Too Large";
        case 414:   return "Request-URI Too Long";
        case 415:   return "Unsupported Media Type";
        case 422:   return "Unprocessable Entity";

        case 500:   return "Internal Server Error";

        // hrhr
        default:    return "Unknown Response Status";
    }
}

static const struct http_content_types {
  const char *name;
  enum http_content_type value;
} http_content_types[] = {
  { "text/plain",                         HTTP_CONTENT_TYPE_TEXT_PLAIN                          },
  { "text/html",                          HTTP_CONTENT_TYPE_TEXT_HTML                           },
  { "application/x-www-form-urlencoded",  HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED   },
  { "application/json",                   HTTP_CONTENT_TYPE_APPLICATION_JSON                    },
  {}
};

enum http_content_type http_content_type_parse (const char *name)
{
  const char *sep = strchr(name, ';'); // ignore any `;charset=...` parameter

  for (const struct http_content_types *ct = http_content_types; ct->name; ct++) {
    if (sep) {
      if (strncasecmp(name, ct->name, (sep - name)) == 0) {
        return ct->value;
      }
    } else {
      if (strcasecmp(name, ct->name) == 0) {
        return ct->value;
      }
    }
  }

  return HTTP_CONTENT_TYPE_UNKNOWN;
}

time_t http_date_parse (const char *value)
{
  struct tm tm;
  const char *p;

  if (!(p = strptime(value, HTTP_DATE_FORMAT, &tm))) {
    return -1;
  } else if (*p) {
    // trailing data
    return -1;
  } else {
    return mktime(&tm);
  }
}

int http_date_format (char *buf, size_t size, time_t time)
{
  struct tm tm;

  if (!gmtime_r(&time, &tm)) {
    return -1;
  }

  if (!strftime(buf, size, HTTP_DATE_FORMAT, &tm)) {
    return -1;
  }

  return 0;
}

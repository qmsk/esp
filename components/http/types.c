#include "http/http_types.h"

#include <strings.h>

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
        case 403:   return "Forbidden";
        case 404:   return "Not Found";
        case 405:   return "Method Not Allowed";
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

enum http_content_type http_content_type_parse (const char *value)
{
  if (strcasecmp(value, "text/plain") == 0) {
      return HTTP_CONTENT_TYPE_TEXT_PLAIN;
  }

  if (strcasecmp(value, "text/html") == 0) {
      return HTTP_CONTENT_TYPE_TEXT_HTML;
  }

  if (strcasecmp(value, "application/x-www-form-urlencoded") == 0) {
      return HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED;
  }

  if (strcasecmp(value, "application/json") == 0) {
      return HTTP_CONTENT_TYPE_APPLICATION_JSON;
  }

  return HTTP_CONTENT_TYPE_UNKNOWN;
}

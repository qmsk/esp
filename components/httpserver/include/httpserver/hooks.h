#ifndef HTTP_HOOKS_H
#define HTTP_HOOKS_H

#include "request.h"
#include "response.h"

enum http_hook_type {
  HTTP_HOOK_REQUEST,
  HTTP_HOOK_REQUEST_HEADER,
  HTTP_HOOK_REQUEST_HEADERS,
  HTTP_HOOK_REQUEST_RESPONSE,

  HTTP_HOOK_RESPONSE,
  HTTP_HOOK_RESPONSE_HEADER,
  HTTP_HOOK_RESPONSE_HEADERS,

  HTTP_HOOK_MAX
};

struct http_hook {
  union http_hook_funcs {
    int (*request)(struct http_request *request, const char *method, const char *path, const char *version, void *ctx);
    int (*request_header)(struct http_request *request, const char *header, const char *value, void *ctx);
    int (*request_headers)(struct http_request *request, void *ctx);
    int (*request_response)(struct http_request *request, struct http_response *response, void *ctx);

    int (*response)(struct http_response *response, enum http_status status, const char *reason, void *ctx);
    int (*response_header)(struct http_response *response, const char *name, void *ctx);
    int (*response_headers)(struct http_response *response, void *ctx);
  } func;
  void *ctx;
};

struct http_hooks {
  const struct http_hook *types[HTTP_HOOK_MAX];
};

#define HTTP_HOOK_RETURN(hooks, type, method, ...) do { \
  int _hook_err; \
  if ((hooks) && (hooks)->types[type]) { \
    if ((_hook_err = (hooks)->types[type]->func.method(__VA_ARGS__, (hooks)->types[type]->ctx))) { \
      return _hook_err; \
    } \
  } \
  return 0; \
} while(0)

#define HTTP_HOOK_CHECK_RETURN(hooks, type, method, ...) do { \
  int _hook_err; \
  if ((hooks) && (hooks)->types[type]) { \
    if ((_hook_err = (hooks)->types[type]->func.method(__VA_ARGS__, (hooks)->types[type]->ctx))) { \
      return _hook_err; \
    } \
  } \
} while(0)

#endif

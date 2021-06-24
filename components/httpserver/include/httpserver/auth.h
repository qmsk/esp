#ifndef HTTP_AUTH_H
#define HTTP_AUTH_H

#include <stddef.h>

int http_basic_authorization (const char *authorization, char *buf, size_t len, const char **usernamep, const char **passwordp);

#endif

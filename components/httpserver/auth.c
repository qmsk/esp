#include "httpserver/auth.h"

#include <logging.h>
#include <mbedtls/base64.h>

#include <ctype.h>
#include <string.h>

#define HTTP_BASIC_AUTHORIZATION "Basic"

int http_basic_authorization (const char *authorization, char *buf, size_t size, const char **usernamep, const char **passwordp)
{
  char *ptr = buf;
  size_t len;
  int err;

  *usernamep = NULL;
  *passwordp = NULL;

  for (const char *prefix = HTTP_BASIC_AUTHORIZATION; *prefix; prefix++) {
    if (*authorization && tolower(*authorization) != tolower(*prefix)) {
      LOG_WARN("Invalid prefix");
      return 400;
    }

    authorization++;
  }

  if (*authorization++ != ' ') {
    LOG_WARN("Invalid separator");
    return 400;
  }

  if ((err = mbedtls_base64_decode((unsigned char *) buf, size, &len, (const unsigned char *) authorization, strlen(authorization)))) {
    LOG_ERROR("mbedtls_base64_decode: %d", err);
    return 400;
  }

  for (; ptr < buf + len; ptr++) {
    if (*ptr == ':' && *usernamep == NULL) {
      *ptr = '\0';
      *usernamep = buf;
    } else if (*usernamep && *passwordp == NULL) {
      *passwordp = ptr;
    }
  }

  if (*usernamep == NULL) {
    LOG_WARN("missing username");
    return 400;
  }

  if (*passwordp == NULL) {
    LOG_WARN("missing password");
    return 400;
  }

  if (ptr >= buf + size) {
    LOG_WARN("Buffer too small");
    return -1;
  }

  *ptr = '\0';

  return 0;
}

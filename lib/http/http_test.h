#ifndef HTTP_TEST_H
#define HTTP_TEST_H

#include "http.h"

/*
 * Parse response from line.
 */
int http_parse_response (char *line, const char **versionp, unsigned *statusp, const char **reasonp);

/*
 * Parse header from line.
 *
 * For a folded header (continuing the previous header), *headerp is left as-is.
 */
int http_parse_header (char *line, const char **headerp, const char **valuep);

#endif

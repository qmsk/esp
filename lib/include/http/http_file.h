#ifndef HTTP_FILE_H
#define HTTP_FILE_H

#include <lib/http/http.h>

 /*
  * Send a HTTP request body from a file.
  *
  * content_length, if given, indicates the expected maximum size of the file to send, or until EOF otherwise.
  *
  * Returns 1 on (unexpected) EOF, <0 on error.
  */
int http_sendfile (struct http *http, int fd, size_t content_length);

/*
 * Read the response body into FILE, or discard if -1.
 *
 * If content_length is given as 0, reads all content.
 */
int http_read_file (struct http *http, int fd, size_t content_length);

/*
 * Read response body chunks into FILE, or discard if -1
 */
int http_read_chunked_file (struct http *http, int fd);

#endif

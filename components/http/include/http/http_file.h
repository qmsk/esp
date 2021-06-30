#ifndef HTTP_FILE_H
#define HTTP_FILE_H

#include <http/http.h>

#include <stdio.h>

enum {
  HTTP_STREAM_READ                = 0x01,
  HTTP_STREAM_WRITE               = 0x02,
};

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

/*
 * Open an stdio FILE to read/write the HTTP request/response.
 *
 * @param http context
 * @param read_content_length limit read request length, update bytes remaining
 * @param flags HTTP_STREAM_READ/WRITE
 * @param filep returned FILE *
 *
 * @return 0 on success, <0 on error
 */
 int http_file_open (struct http *http, int flags, size_t *read_content_length, FILE **filep);

#endif

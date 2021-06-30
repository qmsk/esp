#ifndef _HTTP_H
#define _HTTP_H

#include "http/http.h"

struct http {
    /* Stream IO */
    struct stream *read, *write;

    /* Used by http_file_read */
    size_t *read_content_length;

    /* Used by http_read_chunked_file */
    size_t chunk_size;
};

#endif

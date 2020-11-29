#ifndef _HTTP_H
#define _HTTP_H

#include "http/http.h"

struct http {
    /* Stream IO */
    struct stream *read, *write;

    /* Used by FILE io for state */
    size_t content_length, chunk_size;
};

#endif

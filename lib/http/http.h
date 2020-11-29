#ifndef _HTTP_H
#define _HTTP_H

#include "http/http.h"

struct http {
    /* Stream IO */
    struct stream *read, *write;

    size_t chunk_size;
};

#endif

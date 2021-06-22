#ifndef PARSE_H
#define PARSE_H

#include <stddef.h>

/*
 * In-place string parsing.
 */

enum parse_type {
    PARSE_NONE        = 0,
    PARSE_STRING,
    PARSE_INT,
    PARSE_UINT,


    /* Flags */
    PARSE_TYPE        = 0x0f,

    // do not terminate
    PARSE_KEEP        = 0x10,

    // keep token
    PARSE_SKIP        = 0x20,
};

struct parse {
    /* From state */
    int state;

    /* For char, or -1 for wildcard */
    int c;

    /* To state */
    int next_state;

    /* Store token */
    enum parse_type type;

    union {
        const char **parse_string;

        int *parse_int;

        unsigned *parse_uint;
    };
};

/*
 * Parse a string in-place, per given parse state machine.
 */
int parse (const struct parse *parsing, char *str, int state);

/*
 * Parse a token from a string into a buffer
 */
int tokenize (char *buf, size_t bufsize, const struct parse *parsing, const char **strp, int state);

#endif

#include "parse.h"

#include "logging.h"

#include <stdio.h>
#include <stdlib.h>

/* Lookup parse state for given state/char */
const struct parse * parse_step (const struct parse *parsing, int state, char c)
{
    const struct parse *p;

    for (p = parsing; p->state || p->c || p->next_state; p++) {
        if (p->state == state && p->c == c) {
            return p;
        }

        // wildcard
        if (p->state == state && p->c == -1) {
            return p;
        }
    }

    return NULL;
}

/* Write out a parsed token */
int parse_store (const struct parse *parse, char *token)
{
    switch (parse->type & PARSE_TYPE) {
        case PARSE_NONE:
            return 1;

        case PARSE_STRING:
            *parse->parse_string = token;

            return 0;

        case PARSE_INT:
            if (sscanf(token, "%d", parse->parse_int) != 1) {
                LOG_WARN("invalid int token: %s", token);
                return -1;
            }

            return 0;

        case PARSE_UINT:
            if (sscanf(token, "%u", parse->parse_int) != 1) {
                LOG_WARN("invalid int token: %s", token);
                return -1;
            }

            return 0;

        default:
            // not used
            LOG_WARN("invalid parse type: %#x", parse->type);
            return -1;
    }
}

int parse (const struct parse *parsing, char *str, int state)
{
    char c, *strp = str, *token = str;
    const struct parse *p;
    int err;

    while ((c = *strp)) {
        if (!(p = parse_step(parsing, state, c))) {
            // token continues
            strp++;
            continue;
        }

        // terminiate token?
        if (!(p->type & PARSE_KEEP)) {
            // end current token
            *strp = '\0';
        }

        if ((err = parse_store(p, token)) < 0)
            return err;

        if (err) {
            LOG_DEBUG("%d <- '%c' %d : %s", p->next_state, c, state, token);
        } else {
            LOG_DEBUG("%d <- '%c' %d = %s", p->next_state, c, state, token);
        }

        // begin next token
        state = p->next_state;

        if ((p->type & PARSE_SKIP)) {
            // keep the token pointing to the previous token, terminated by this char
            strp++;
        } else if ((p->type & PARSE_KEEP)) {
            // this non-terminated char is the start of the token
            token = strp++;
        } else {
            // begin next token after this terminating char
            token = ++strp;
        }
    }

    // terminate
    if ((p = parse_step(parsing, state, *strp))) {
        LOG_DEBUG("%d <-     %d = %s", p->next_state, state, token);

        state = p->next_state;

        if ((err = parse_store(p, token)))
            return err;
    }

    return state;
}

int tokenize (char *buf, size_t bufsize, const struct parse *parsing, const char **strp, int state)
{
    char *out = buf, c;
    const char *str = *strp;
    const struct parse *p;

    while ((c = *str++)) {
        p = parse_step(parsing, state, c);

        if (p) {
            state = p->next_state;
        }

        if (!p || (p->type & PARSE_KEEP)) {
            // token continues
            *out++ = c;

        } else if (p && (p->type & PARSE_SKIP)) {
            // omit

        } else {
            // end of token
            break;
        }

        if (out >= buf + bufsize) {
            LOG_WARN("token too long");
            return -1;
        }
    }

    *out = '\0';
    *strp = str;

    return state;
}

#ifndef HTTP_TYPES_H
#define HTTP_TYPES_H

enum http_version {
    HTTP_10 = 0,    // default
    HTTP_11,
};

enum http_status {
    HTTP_OK                       = 200,
    HTTP_CREATED                  = 201,
    HTTP_NO_CONTENT               = 204,
    HTTP_FOUND                    = 301,
    HTTP_BAD_REQUEST              = 400,
    HTTP_FORBIDDEN                = 403,
    HTTP_NOT_FOUND                = 404,
    HTTP_METHOD_NOT_ALLOWED       = 405,
    HTTP_LENGTH_REQUIRED          = 411,
    HTTP_REQUEST_ENTITY_TOO_LARGE = 413,
    HTTP_REQUEST_URI_TOO_LONG     = 414,
    HTTP_UNSUPPORTED_MEDIA_TYPE   = 415,
    HTTP_UNPROCESSABLE_ENTITY     = 422,
    HTTP_INTERNAL_SERVER_ERROR    = 500,
};

enum http_content_type {
    HTTP_CONTENT_TYPE_UNKNOWN     = 0,

    HTTP_CONTENT_TYPE_TEXT_PLAIN,
    HTTP_CONTENT_TYPE_TEXT_HTML,

    HTTP_CONTENT_TYPE_APPLICATION_X_WWW_FORM_URLENCODED,
    HTTP_CONTENT_TYPE_APPLICATION_JSON,
};

/*
 * Return a const char* with a textual http version.
 */
const char *http_version_str (enum http_version version);

/*
 * Return a const char* with a textual reason for the given http status.
 */
const char *http_status_str (enum http_status status);

/*
 * Return an enum value for the content type
 */
enum http_content_type http_content_type_parse (const char *value);


#endif

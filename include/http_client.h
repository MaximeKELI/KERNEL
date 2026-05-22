#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include "types.h"
#include "net_addr.h"

#define HTTP_RESP_MAX 4096

typedef struct http_response {
    int status_code;
    char body[HTTP_RESP_MAX];
    size_t body_len;
} http_response_t;

int http_get(const ip_addr_t* host, u16 port, const char* path, http_response_t* resp);
int http_get_url(const char* host_str, u16 port, const char* path, http_response_t* resp);

#endif /* HTTP_CLIENT_H */

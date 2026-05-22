#include "http_client.h"
#include "net.h"
#include "net_addr.h"
#include "net_socket.h"
#include "tcp.h"
#include "string.h"
#include "stdio.h"
#include "ip.h"
#include "debug.h"

static int http_wait_established(socket_t* sock, u32 polls) {
    for (u32 i = 0; i < polls; i++) {
        net_poll();
        if (tcp_socket_established(sock)) {
            return 0;
        }
    }
    return -1;
}

static void http_fill_sockaddr(sockaddr_t* addr, const ip_addr_t* host, u16 port) {
    addr->sa_family = AF_INET;
    u16 port_be = htons(port);
    memcpy(addr->sa_data, &port_be, 2);
    memcpy(addr->sa_data + 2, host->addr, 4);
}

int http_get(const ip_addr_t* host, u16 port, const char* path, http_response_t* resp) {
    if (!host || !path || !resp) {
        return -1;
    }
    memset(resp, 0, sizeof(*resp));

    socket_t* sock = socket_create(SOCK_STREAM, IPPROTO_TCP);
    if (!sock) {
        return -1;
    }

    sockaddr_t addr;
    memset(&addr, 0, sizeof(addr));
    http_fill_sockaddr(&addr, host, port);

    if (tcp_ops.connect(sock, &addr) < 0) {
        socket_destroy(sock);
        return -1;
    }

    if (http_wait_established(sock, 512) < 0) {
        socket_destroy(sock);
        return -1;
    }

    char req[512];
    snprintf(req, sizeof(req),
             "GET %s HTTP/1.0\r\nHost: gateway\r\nConnection: close\r\n\r\n",
             path[0] ? path : "/");

    if (tcp_ops.send(sock, req, strlen(req)) < 0) {
        socket_destroy(sock);
        return -1;
    }

    char buf[HTTP_RESP_MAX];
    size_t total = 0;
    for (u32 i = 0; i < 1024 && total < sizeof(buf) - 1; i++) {
        net_poll();
        ssize_t n = tcp_ops.recv(sock, buf + total, sizeof(buf) - 1 - total);
        if (n > 0) {
            total += (size_t)n;
        } else if (n < 0) {
            break;
        }
    }
    buf[total] = '\0';
    socket_destroy(sock);

    const char* status = strstr(buf, "HTTP/");
    if (status) {
        const char* code = strchr(status, ' ');
        if (code) {
            resp->status_code = atoi(code + 1);
        }
    }

    const char* body = strstr(buf, "\r\n\r\n");
    if (body) {
        body += 4;
        size_t blen = total - (size_t)(body - buf);
        if (blen >= HTTP_RESP_MAX) {
            blen = HTTP_RESP_MAX - 1;
        }
        memcpy(resp->body, body, blen);
        resp->body[blen] = '\0';
        resp->body_len = blen;
    }
    return resp->status_code > 0 ? 0 : -1;
}

int http_get_url(const char* host_str, u16 port, const char* path, http_response_t* resp) {
    ip_addr_t ip;
    if (!ip_addr_parse(host_str, &ip)) {
        return -1;
    }
    return http_get(&ip, port, path, resp);
}

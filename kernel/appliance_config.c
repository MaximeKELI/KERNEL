#include "appliance_config.h"
#include "net_addr.h"
#include "string.h"
#include "stdio.h"

static appliance_config_t g_cfg;

static int parse_u8_field(const char* s, u8* out) {
    u32 v = 0;
    while (*s >= '0' && *s <= '9') {
        v = v * 10 + (u32)(*s - '0');
        s++;
    }
    *out = (u8)(v > 255 ? 255 : v);
    return 0;
}

static int parse_ip(const char* s, ip_addr_t* out) {
    if (!s || !out) {
        return -1;
    }
    parse_u8_field(s, &out->addr[0]);
    while (*s && *s != '.') {
        s++;
    }
    if (*s == '.') {
        s++;
    }
    parse_u8_field(s, &out->addr[1]);
    while (*s && *s != '.') {
        s++;
    }
    if (*s == '.') {
        s++;
    }
    parse_u8_field(s, &out->addr[2]);
    while (*s && *s != '.') {
        s++;
    }
    if (*s == '.') {
        s++;
    }
    parse_u8_field(s, &out->addr[3]);
    return 0;
}

void appliance_config_init(void) {
    g_cfg.gateway.addr[0] = 10;
    g_cfg.gateway.addr[1] = 0;
    g_cfg.gateway.addr[2] = 2;
    g_cfg.gateway.addr[3] = 2;
    g_cfg.dns_server.addr[0] = 8;
    g_cfg.dns_server.addr[1] = 8;
    g_cfg.dns_server.addr[2] = 8;
    g_cfg.dns_server.addr[3] = 8;
    strncpy(g_cfg.hostname, "kernel-appliance", sizeof(g_cfg.hostname) - 1);
    g_cfg.http_port = 80;
    g_cfg.dhcp_enabled = true;
}

const appliance_config_t* appliance_config_get(void) {
    return &g_cfg;
}

int appliance_config_load(const char* path) {
    extern ssize_t vfs_read_path(const char* path, void* buf, size_t count);
    char blob[512];
    ssize_t n = vfs_read_path(path, blob, sizeof(blob) - 1);
    if (n <= 0) {
        return -1;
    }
    blob[n] = '\0';

    char* p = blob;
    while (*p) {
        char* e = strchr(p, '\n');
        if (e) {
            *e = '\0';
        }
        if (strncmp(p, "gateway=", 8) == 0) {
            parse_ip(p + 8, &g_cfg.gateway);
        } else if (strncmp(p, "dns=", 4) == 0) {
            parse_ip(p + 4, &g_cfg.dns_server);
        } else if (strncmp(p, "hostname=", 9) == 0) {
            strncpy(g_cfg.hostname, p + 9, sizeof(g_cfg.hostname) - 1);
        } else if (strncmp(p, "http_port=", 10) == 0) {
            u32 port = 0;
            const char* s = p + 10;
            while (*s >= '0' && *s <= '9') {
                port = port * 10 + (u32)(*s - '0');
                s++;
            }
            g_cfg.http_port = (u16)port;
        } else if (strncmp(p, "dhcp=", 5) == 0) {
            g_cfg.dhcp_enabled = (p[5] == '1' || p[5] == 'y');
        }
        if (!e) {
            break;
        }
        p = e + 1;
    }
    return 0;
}

int appliance_config_save(const char* path) {
    extern ssize_t vfs_write_path(const char* path, const void* buf, size_t count);
    char buf[256];
    char gw[16], dns[16];
    ip_addr_format(&g_cfg.gateway, gw, sizeof(gw));
    ip_addr_format(&g_cfg.dns_server, dns, sizeof(dns));
    snprintf(buf, sizeof(buf),
             "gateway=%s\ndns=%s\nhostname=%s\nhttp_port=%u\ndhcp=%u\n",
             gw, dns, g_cfg.hostname, g_cfg.http_port,
             g_cfg.dhcp_enabled ? 1u : 0u);
    return vfs_write_path(path, buf, strlen(buf)) >= 0 ? 0 : -1;
}

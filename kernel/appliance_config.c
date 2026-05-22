#include "appliance_config.h"
#include "string.h"
#include "stdio.h"

static appliance_config_t g_cfg;

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

static int parse_ip(const char* s, ip_addr_t* out) {
    u32 a = 0, b = 0, c = 0, d = 0;
    if (!s || !out) {
        return -1;
    }
    if (sscanf(s, "%u.%u.%u.%u", &a, &b, &c, &d) != 4) {
        return -1;
    }
    out->addr[0] = (u8)a;
    out->addr[1] = (u8)b;
    out->addr[2] = (u8)c;
    out->addr[3] = (u8)d;
    return 0;
}

int appliance_config_load(const char* path) {
    extern ssize_t vfs_read_path(const char* path, void* buf, size_t count);
    char line[128];
    ssize_t n = vfs_read_path(path, line, sizeof(line) - 1);
    if (n <= 0) {
        return -1;
    }
    line[n] = '\0';

    char* p = line;
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
            g_cfg.http_port = (u16)atoi(p + 10);
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

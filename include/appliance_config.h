#ifndef APPLIANCE_CONFIG_H
#define APPLIANCE_CONFIG_H

#include "types.h"
#include "net_addr.h"

typedef struct appliance_config {
    ip_addr_t gateway;
    ip_addr_t dns_server;
    char hostname[32];
    u16 http_port;
    bool dhcp_enabled;
} appliance_config_t;

void appliance_config_init(void);
const appliance_config_t* appliance_config_get(void);
int appliance_config_load(const char* path);
int appliance_config_save(const char* path);

#endif /* APPLIANCE_CONFIG_H */

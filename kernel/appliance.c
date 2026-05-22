#include "appliance.h"
#include "kernel_init.h"
#include "appliance_config.h"
#include "http_client.h"
#include "net.h"
#include "net_addr.h"
#include "icmp.h"
#include "dhcp.h"
#include "stdio.h"

void appliance_network_boot(void) {
    if (!kernel_extended_ready()) {
        printk("[appliance] init-full required\n");
        return;
    }

    const appliance_config_t* cfg = appliance_config_get();
    appliance_config_load("/etc/appliance.conf");

    netif_t* iface = net_default_if();
    if (!iface) {
        printk("[appliance] no interface\n");
        return;
    }

    printk("[appliance] host=%s\n", cfg->hostname);
    if (cfg->dhcp_enabled) {
        net_dhcp_acquire(iface);
    }

    icmp_ping_reset_stats();
    icmp_ping(cfg->gateway, 42, 1);
    for (u32 i = 0; i < 256; i++) {
        net_poll();
    }

    u32 replies = icmp_ping_replies_received();
    printk("[appliance] ping gateway: %u reply(s)\n", replies);

    http_response_t http;
    if (http_get(&cfg->gateway, cfg->http_port, "/", &http) == 0) {
        printk("[appliance] HTTP GET: status %d (%zu bytes body)\n",
               http.status_code, http.body_len);
    } else {
        printk("[appliance] HTTP GET failed (gateway may not run HTTP)\n");
    }
}

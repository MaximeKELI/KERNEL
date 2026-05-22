#include "appliance.h"
#include "kernel_init.h"
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

    netif_t* iface = net_default_if();
    if (!iface) {
        printk("[appliance] no interface\n");
        return;
    }

    printk("[appliance] network appliance boot\n");
    net_dhcp_acquire(iface);

    ip_addr_t gw = {{10, 0, 2, 2}};
    icmp_ping_reset_stats();
    icmp_ping(gw, 42, 1);
    for (u32 i = 0; i < 256; i++) {
        net_poll();
    }

    u32 replies = icmp_ping_replies_received();
    printk("[appliance] ping 10.0.2.2: %u reply(s)\n", replies);
    if (replies > 0) {
        printk("[appliance] QEMU user networking OK\n");
    } else {
        printk("[appliance] no reply (try: make run, init-full, dhcp, ping 10.0.2.2)\n");
    }
}

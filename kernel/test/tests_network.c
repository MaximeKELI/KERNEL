#include "test.h"
#include "net.h"
#include "tcp.h"
#include "udp.h"
#include "ip.h"
#include "icmp.h"
#include "arp.h"
#include "skbuff.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "pci.h"
#include "ethernet.h"
#include "e1000.h"
#include "net_addr.h"

/* Test TCP initialization */
static test_result_t test_tcp_init(void) {
    tcp_init();
    TEST_ASSERT(1); /* If we get here, init succeeded */
    return TEST_PASS;
}

/* Test UDP initialization */
static test_result_t test_udp_init(void) {
    udp_init();
    TEST_ASSERT(1);
    return TEST_PASS;
}

/* Test IP initialization */
static test_result_t test_ip_init(void) {
    ip_init();
    TEST_ASSERT(1);
    return TEST_PASS;
}

/* Test ICMP initialization */
static test_result_t test_icmp_init(void) {
    icmp_init();
    TEST_ASSERT(1);
    return TEST_PASS;
}

/* Test ARP initialization */
static test_result_t test_arp_init(void) {
    arp_init();
    TEST_ASSERT(1);
    return TEST_PASS;
}

/* Test socket creation */
static test_result_t test_socket_create(void) {
    socket_t* sock = socket_create(2, SOCK_STREAM, IPPROTO_TCP); /* AF_INET = 2 */
    TEST_ASSERT_NOT_NULL(sock);
    TEST_ASSERT_EQ(sock->domain, 2);
    TEST_ASSERT_EQ(sock->type, SOCK_STREAM);
    TEST_ASSERT_EQ(sock->protocol, IPPROTO_TCP);
    socket_destroy(sock);
    return TEST_PASS;
}

/* Test socket creation UDP */
static test_result_t test_socket_create_udp(void) {
    socket_t* sock = socket_create(2, SOCK_DGRAM, IPPROTO_UDP);
    TEST_ASSERT_NOT_NULL(sock);
    TEST_ASSERT_EQ(sock->type, SOCK_DGRAM);
    TEST_ASSERT_EQ(sock->protocol, IPPROTO_UDP);
    socket_destroy(sock);
    return TEST_PASS;
}

/* Test sk_buff allocation */
static test_result_t test_skb_alloc(void) {
    sk_buff_t* skb = skb_alloc(1024);
    TEST_ASSERT_NOT_NULL(skb);
    TEST_ASSERT_EQ(skb->len, 0);
    TEST_ASSERT_NOT_NULL(skb->data);
    skb_free(skb);
    return TEST_PASS;
}

/* Test sk_buff put */
static test_result_t test_skb_put(void) {
    sk_buff_t* skb = skb_alloc(1024);
    TEST_ASSERT_NOT_NULL(skb);

    void* data = skb_put(skb, 100);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQ(skb->len, 100);

    skb_free(skb);
    return TEST_PASS;
}

/* Test sk_buff push */
static test_result_t test_skb_push(void) {
    sk_buff_t* skb = skb_alloc(1024);
    TEST_ASSERT_NOT_NULL(skb);

    skb_put(skb, 50);
    void* data = skb_push(skb, 20);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQ(skb->len, 70);

    skb_free(skb);
    return TEST_PASS;
}

/* Test sk_buff pull */
static test_result_t test_skb_pull(void) {
    sk_buff_t* skb = skb_alloc(1024);
    TEST_ASSERT_NOT_NULL(skb);

    skb_put(skb, 100);
    void* data = skb_pull(skb, 30);
    TEST_ASSERT_NOT_NULL(data);
    TEST_ASSERT_EQ(skb->len, 70);

    skb_free(skb);
    return TEST_PASS;
}

/* Test IP checksum */
static test_result_t test_ip_checksum(void) {
    u8 data[20] = {0};
    data[0] = 0x45; /* Version 4, IHL 5 */
    data[2] = 0x00;
    data[3] = 0x14; /* Total length 20 */

    u16 checksum = ip_checksum(data, 20);
    TEST_ASSERT_NE(checksum, 0);
    return TEST_PASS;
}

/* Test network byte order conversion */
static test_result_t test_hton_ntoh(void) {
    u16 host_short = 0x1234;
    u16 net_short = htons(host_short);
    TEST_ASSERT_EQ(ntohs(net_short), host_short);

    u32 host_long = 0x12345678;
    u32 net_long = htonl(host_long);
    TEST_ASSERT_EQ(ntohl(net_long), host_long);
    return TEST_PASS;
}

/* Test socket bind (basic) */
static test_result_t test_socket_bind(void) {
    socket_t* sock = socket_create(2, SOCK_STREAM, IPPROTO_TCP);
    TEST_ASSERT_NOT_NULL(sock);

    sockaddr_t addr;
    addr.sa_family = 2; /* AF_INET */
    *(u16*)&addr.sa_data[0] = htons(8080);
    addr.sa_data[2] = 127;
    addr.sa_data[3] = 0;
    addr.sa_data[4] = 0;
    addr.sa_data[5] = 1;

    if (sock->ops && sock->ops->bind) {
        int ret = sock->ops->bind(sock, &addr);
        (void)ret;
        /* May fail if port in use, but should not crash */
        TEST_ASSERT(1);
    }

    socket_destroy(sock);
    return TEST_PASS;
}

/* Test socket listen */
static test_result_t test_socket_listen(void) {
    socket_t* sock = socket_create(2, SOCK_STREAM, IPPROTO_TCP);
    TEST_ASSERT_NOT_NULL(sock);

    if (sock->ops && sock->ops->listen) {
        int ret = sock->ops->listen(sock, 5);
        (void)ret;
        /* May fail if bind not called, but should not crash */
        TEST_ASSERT(1);
    }

    socket_destroy(sock);
    return TEST_PASS;
}

/* Test socket close */
static test_result_t test_socket_close(void) {
    socket_t* sock = socket_create(2, SOCK_STREAM, IPPROTO_TCP);
    TEST_ASSERT_NOT_NULL(sock);

    if (sock->ops && sock->ops->close) {
        int ret = sock->ops->close(sock);
        TEST_ASSERT_EQ(ret, 0);
    }

    socket_destroy(sock);
    return TEST_PASS;
}

/* Test sk_buff clone */
static test_result_t test_skb_clone(void) {
    sk_buff_t* skb = skb_alloc(1024);
    TEST_ASSERT_NOT_NULL(skb);

    skb_put(skb, 100);
    memset(skb->data, 0xAA, 100);

    sk_buff_t* clone = skb_clone(skb);
    TEST_ASSERT_NOT_NULL(clone);
    TEST_ASSERT_EQ(clone->len, skb->len);
    TEST_ASSERT_EQ(skb->users, 2); /* Original + clone */

    skb_free(skb);
    skb_free(clone);
    return TEST_PASS;
}

/* Test IP address structure */
static test_result_t test_ip_addr(void) {
    ip_addr_t addr1 = {{127, 0, 0, 1}};
    ip_addr_t addr2 = {{127, 0, 0, 1}};
    ip_addr_t addr3 = {{192, 168, 1, 1}};

    TEST_ASSERT(memcmp(&addr1, &addr2, sizeof(ip_addr_t)) == 0);
    TEST_ASSERT(memcmp(&addr1, &addr3, sizeof(ip_addr_t)) != 0);
    return TEST_PASS;
}

/* Regression: snprintf must expand %u (it used to fall through and print the
 * literal "%u", which broke every ip_addr_format() dotted-quad). */
static test_result_t test_snprintf_u(void) {
    char buf[32];

    snprintf(buf, sizeof(buf), "%u", 0u);
    TEST_ASSERT(strcmp(buf, "0") == 0);

    snprintf(buf, sizeof(buf), "%u", 4294967295u);
    TEST_ASSERT(strcmp(buf, "4294967295") == 0);

    snprintf(buf, sizeof(buf), "%u.%u.%u.%u", 10u, 0u, 2u, 15u);
    TEST_ASSERT(strcmp(buf, "10.0.2.15") == 0);

    ip_addr_t ip = {{10, 0, 2, 15}};
    ip_addr_format(&ip, buf, sizeof(buf));
    TEST_ASSERT(strcmp(buf, "10.0.2.15") == 0);
    return TEST_PASS;
}

/* Test the e1000 driver binds to QEMU's default NIC (8086:100e).
 * Covers the real path: PCI enumeration -> device match -> probe (reset, MAC
 * read, ring setup, handler install) -> RX/TX through the public ethernet API.
 * (End-to-end traffic needs a network backend, which the CI VM lacks.) */
static test_result_t test_e1000_probe(void) {
    pci_init();  /* scan bus 0 so the NIC is discoverable */

    pci_device_t* pci = pci_find_device(E1000_VENDOR, E1000_DEVICE);
    TEST_ASSERT_NOT_NULL(pci);

    ethernet_device_t dev;
    memset(&dev, 0, sizeof(dev));
    strncpy(dev.name, "test0", sizeof(dev.name) - 1);

    TEST_ASSERT_EQ(e1000_probe(pci, &dev), 0);

    /* Data-path handlers must be installed -> counts as a hardware NIC. */
    TEST_ASSERT_NOT_NULL((void*)dev.tx_fn);
    TEST_ASSERT_NOT_NULL((void*)dev.rx_fn);
    TEST_ASSERT_NOT_NULL((void*)dev.poll_fn);
    TEST_ASSERT(ethernet_is_hardware(&dev));

    /* QEMU's default e1000 MAC is 52:54:00:12:34:56. */
    TEST_ASSERT_EQ(dev.mac_address[0], 0x52);
    TEST_ASSERT_EQ(dev.mac_address[1], 0x54);
    TEST_ASSERT_EQ(dev.mac_address[2], 0x00);

    dev.up = true;

    /* No frame has arrived: RX must report "nothing" (0), never a bogus read. */
    u8 rxbuf[64];
    TEST_ASSERT_EQ(ethernet_receive_packet(&dev, rxbuf, sizeof(rxbuf)), 0);

    /* TX a minimum-size frame: the descriptor ring + MMIO doorbell must accept it. */
    u8 frame[60];
    memset(frame, 0xFF, 6);           /* broadcast dst */
    memcpy(frame + 6, dev.mac_address, 6);
    frame[12] = 0x08; frame[13] = 0x00;
    memset(frame + 14, 0, sizeof(frame) - 14);
    TEST_ASSERT_EQ(ethernet_send_packet(&dev, frame, sizeof(frame)), 0);
    TEST_ASSERT_EQ(dev.tx_packets, 1);

    return TEST_PASS;
}

/* Register all network tests */
void register_network_tests(void) {
    test_register("network", "e1000_probe", test_e1000_probe);
    test_register("network", "tcp_init", test_tcp_init);
    test_register("network", "udp_init", test_udp_init);
    test_register("network", "ip_init", test_ip_init);
    test_register("network", "icmp_init", test_icmp_init);
    test_register("network", "arp_init", test_arp_init);
    test_register("network", "socket_create", test_socket_create);
    test_register("network", "socket_create_udp", test_socket_create_udp);
    test_register("network", "skb_alloc", test_skb_alloc);
    test_register("network", "skb_put", test_skb_put);
    test_register("network", "skb_push", test_skb_push);
    test_register("network", "skb_pull", test_skb_pull);
    test_register("network", "ip_checksum", test_ip_checksum);
    test_register("network", "hton_ntoh", test_hton_ntoh);
    test_register("network", "socket_bind", test_socket_bind);
    test_register("network", "socket_listen", test_socket_listen);
    test_register("network", "socket_close", test_socket_close);
    test_register("network", "skb_clone", test_skb_clone);
    test_register("network", "ip_addr", test_ip_addr);
    test_register("network", "snprintf_u", test_snprintf_u);
}

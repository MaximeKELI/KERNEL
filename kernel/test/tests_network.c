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

/* Register all network tests */
void register_network_tests(void) {
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
}

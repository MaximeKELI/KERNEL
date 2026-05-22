#include "eth.h"
#include "ethernet.h"
#include "net.h"
#include "net_stats.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "spinlock.h"
#include "types.h"

#define ETH_LOOP_QUEUE 32

typedef struct {
    char ifname[16];
    size_t len;
    u8 data[ETH_FRAME_MAX];
} eth_loop_frame_t;

static eth_loop_frame_t loop_queue[ETH_LOOP_QUEUE];
static u32 loop_head = 0;
static u32 loop_tail = 0;
static u32 loop_count = 0;
static spinlock_t loop_lock = SPINLOCK_INIT;

void eth_loopback_init(void) {
    loop_head = loop_tail = loop_count = 0;
    memset(loop_queue, 0, sizeof(loop_queue));
}

static int eth_loop_enqueue(const char* ifname, const void* frame, size_t len) {
    if (!ifname || !frame || len == 0 || len > ETH_FRAME_MAX) {
        return -1;
    }

    spinlock_lock(&loop_lock);
    if (loop_count >= ETH_LOOP_QUEUE) {
        spinlock_unlock(&loop_lock);
        net_stats_update_tx_error();
        return -1;
    }

    eth_loop_frame_t* slot = &loop_queue[loop_tail];
    strncpy(slot->ifname, ifname, sizeof(slot->ifname) - 1);
    slot->ifname[sizeof(slot->ifname) - 1] = '\0';
    slot->len = len;
    memcpy(slot->data, frame, len);
    loop_tail = (loop_tail + 1) % ETH_LOOP_QUEUE;
    loop_count++;
    spinlock_unlock(&loop_lock);
    return 0;
}

static int eth_loop_dequeue(const char* ifname, void* buf, size_t buf_size) {
    if (!ifname || !buf || buf_size == 0) {
        return -1;
    }

    spinlock_lock(&loop_lock);
    for (u32 i = 0; i < loop_count; i++) {
        u32 idx = (loop_head + i) % ETH_LOOP_QUEUE;
        eth_loop_frame_t* slot = &loop_queue[idx];
        if (strcmp(slot->ifname, ifname) != 0) {
            continue;
        }
        if (slot->len > buf_size) {
            spinlock_unlock(&loop_lock);
            return -1;
        }
        memcpy(buf, slot->data, slot->len);
        size_t len = slot->len;

        for (u32 j = i; j < loop_count - 1; j++) {
            u32 a = (loop_head + j) % ETH_LOOP_QUEUE;
            u32 b = (loop_head + j + 1) % ETH_LOOP_QUEUE;
            loop_queue[a] = loop_queue[b];
        }
        loop_count--;
        if (loop_count == 0) {
            loop_head = loop_tail = 0;
        } else {
            loop_head = (loop_head + 1) % ETH_LOOP_QUEUE;
            if (loop_tail > 0) {
                loop_tail = (loop_tail + ETH_LOOP_QUEUE - 1) % ETH_LOOP_QUEUE;
            }
        }
        spinlock_unlock(&loop_lock);
        return (int)len;
    }
    spinlock_unlock(&loop_lock);
    return 0;
}

int eth_transmit(netif_t* iface, u16 proto, const u8* dst_mac,
                 const void* payload, size_t len) {
    if (!iface || !iface->up || !dst_mac || !payload || len == 0) {
        return -1;
    }
    if (len + ETH_HDR_LEN > ETH_FRAME_MAX) {
        return -1;
    }

    u8 frame[ETH_FRAME_MAX];
    eth_header_t* eth = (eth_header_t*)frame;
    memcpy(eth->dst, dst_mac, ETH_ALEN);
    memcpy(eth->src, iface->mac, ETH_ALEN);
    eth->proto = htons(proto);
    memcpy(frame + ETH_HDR_LEN, payload, len);

    size_t frame_len = ETH_HDR_LEN + len;
    ethernet_device_t* dev = ethernet_find_device(iface->name);
    if (!dev) {
        return -1;
    }

    if (ethernet_send_packet(dev, frame, frame_len) < 0) {
        return -1;
    }

    net_stats_update_tx(frame_len);
    global_net_tx_bytes += frame_len;
    global_net_tx_packets++;
    return 0;
}

int eth_poll(netif_t* iface) {
    if (!iface || !iface->up) {
        return 0;
    }

    u8 buf[ETH_FRAME_MAX];
    int n = eth_loop_dequeue(iface->name, buf, sizeof(buf));
    if (n <= 0) {
        return 0;
    }

    net_stats_update_rx((size_t)n);
    global_net_rx_bytes += (size_t)n;
    global_net_rx_packets++;
    return net_process_packet(iface, buf, (size_t)n);
}

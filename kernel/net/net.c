#include "net.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"

static netif_t* netif_list = NULL;
static socket_t* socket_list = NULL;
static spinlock_t net_lock = SPINLOCK_INIT;

void net_init(void) {
    DEBUG_INFO("Networking stack initialized");
}

socket_t* socket_create(int domain, int type, int protocol) {
    socket_t* sock = (socket_t*)kzalloc(sizeof(socket_t));
    if (!sock) {
        DEBUG_ERROR("Failed to allocate socket");
        return NULL;
    }
    
    sock->domain = domain;
    sock->type = type;
    sock->protocol = protocol;
    
    spinlock_lock(&net_lock);
    sock->private_data = (void*)socket_list;
    socket_list = sock;
    spinlock_unlock(&net_lock);
    
    DEBUG_INFO("Socket created: domain=%d, type=%d, protocol=%d",
               domain, type, protocol);
    return sock;
}

int netif_register(netif_t* iface) {
    if (!iface) return -1;
    
    spinlock_lock(&net_lock);
    iface->next = netif_list;
    netif_list = iface;
    spinlock_unlock(&net_lock);
    
    DEBUG_INFO("Network interface registered: %s", iface->name);
    return 0;
}

int net_send_packet(netif_t* iface, const void* data, size_t len) {
    (void)iface;
    (void)data;
    (void)len;
    /* Would send via network driver */
    return 0;
}

int net_recv_packet(netif_t* iface, void* data, size_t* len) {
    (void)iface;
    (void)data;
    (void)len;
    /* Would receive via network driver */
    return 0;
}

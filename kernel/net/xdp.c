#include "xdp.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

static xdp_prog_t* xdp_programs = NULL;
static spinlock_t xdp_lock = SPINLOCK_INIT;

void xdp_init(void) {
    DEBUG_INFO("XDP (eXpress Data Path) initialized");
}

int xdp_attach_prog(u32 ifindex, bpf_prog_t* prog) {
    if (!prog) return -1;
    
    xdp_prog_t* xdp_prog = (xdp_prog_t*)kzalloc(sizeof(xdp_prog_t));
    if (!xdp_prog) {
        DEBUG_ERROR("Failed to allocate XDP program");
        return -1;
    }
    
    xdp_prog->bpf_prog = prog;
    xdp_prog->ifindex = ifindex;
    
    spinlock_lock(&xdp_lock);
    xdp_prog->next = xdp_programs;
    xdp_programs = xdp_prog;
    spinlock_unlock(&xdp_lock);
    
    DEBUG_INFO("XDP program attached: ifindex=%u", ifindex);
    return 0;
}

int xdp_detach_prog(u32 ifindex) {
    spinlock_lock(&xdp_lock);
    
    xdp_prog_t** prev = &xdp_programs;
    xdp_prog_t* current = xdp_programs;
    
    while (current) {
        if (current->ifindex == ifindex) {
            *prev = current->next;
            kfree(current);
            spinlock_unlock(&xdp_lock);
            DEBUG_INFO("XDP program detached: ifindex=%u", ifindex);
            return 0;
        }
        prev = &current->next;
        current = current->next;
    }
    
    spinlock_unlock(&xdp_lock);
    return -1;
}

u32 xdp_process_packet(u32 ifindex, void* packet, size_t length) {
    (void)packet;
    (void)length;
    
    spinlock_lock(&xdp_lock);
    
    xdp_prog_t* prog = xdp_programs;
    while (prog) {
        if (prog->ifindex == ifindex && prog->bpf_prog) {
            /* Would execute BPF program */
            spinlock_unlock(&xdp_lock);
            return XDP_PASS;
        }
        prog = prog->next;
    }
    
    spinlock_unlock(&xdp_lock);
    return XDP_PASS; /* Default: pass */
}

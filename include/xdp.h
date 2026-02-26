#ifndef XDP_H
#define XDP_H

#include "types.h"
#include "bpf.h"

/* XDP actions */
#define XDP_ABORTED 0
#define XDP_DROP    1
#define XDP_PASS    2
#define XDP_TX      3
#define XDP_REDIRECT 4

/* XDP program */
typedef struct xdp_prog {
    bpf_prog_t* bpf_prog;
    u32 ifindex;
    struct xdp_prog* next;
} xdp_prog_t;

/* Initialize XDP */
void xdp_init(void);

/* Attach XDP program */
int xdp_attach_prog(u32 ifindex, bpf_prog_t* prog);

/* Detach XDP program */
int xdp_detach_prog(u32 ifindex);

/* Process packet with XDP */
u32 xdp_process_packet(u32 ifindex, void* packet, size_t length);

#endif /* XDP_H */

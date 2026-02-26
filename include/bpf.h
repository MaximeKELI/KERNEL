#ifndef BPF_H
#define BPF_H

#include "types.h"

/* BPF program types */
#define BPF_PROG_TYPE_UNSPEC 0
#define BPF_PROG_TYPE_SOCKET_FILTER 1
#define BPF_PROG_TYPE_KPROBE 2
#define BPF_PROG_TYPE_TRACEPOINT 3
#define BPF_PROG_TYPE_XDP 4

/* BPF map types */
#define BPF_MAP_TYPE_UNSPEC 0
#define BPF_MAP_TYPE_HASH 1
#define BPF_MAP_TYPE_ARRAY 2
#define BPF_MAP_TYPE_PROG_ARRAY 3

/* BPF program */
typedef struct bpf_prog {
    u32 type;
    u32 len;
    u64* insns;
    u32 fd;
    bool jited;
    void* bpf_func;
    struct bpf_prog* next;
} bpf_prog_t;

/* BPF map */
typedef struct bpf_map {
    u32 type;
    u32 key_size;
    u32 value_size;
    u32 max_entries;
    u32 map_fd;
    void* data;
    struct bpf_map* next;
} bpf_map_t;

/* Initialize BPF */
void bpf_init(void);

/* Load BPF program */
bpf_prog_t* bpf_prog_load(u32 prog_type, u64* insns, u32 len);

/* Attach BPF program */
int bpf_prog_attach(bpf_prog_t* prog, u32 attach_type, u32 attach_id);

/* Create BPF map */
bpf_map_t* bpf_map_create(u32 map_type, u32 key_size, u32 value_size, u32 max_entries);

/* Update BPF map */
int bpf_map_update_elem(bpf_map_t* map, const void* key, const void* value);

/* Lookup BPF map */
int bpf_map_lookup_elem(bpf_map_t* map, const void* key, void* value);

#endif /* BPF_H */

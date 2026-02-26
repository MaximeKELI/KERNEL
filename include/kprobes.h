#ifndef KPROBES_H
#define KPROBES_H

#include "types.h"

/* Kprobe */
typedef struct kprobe {
    void* addr;
    void (*pre_handler)(struct kprobe* kp, void* regs);
    void (*post_handler)(struct kprobe* kp, void* regs, u64 flags);
    void (*fault_handler)(struct kprobe* kp, void* regs, u32 trapnr);
    bool active;
    struct kprobe* next;
} kprobe_t;

/* Initialize kprobes */
void kprobes_init(void);

/* Register kprobe */
int register_kprobe(kprobe_t* kp);

/* Unregister kprobe */
void unregister_kprobe(kprobe_t* kp);

/* Jprobe (jump probe) */
typedef struct jprobe {
    kprobe_t kp;
    void* entry;
} jprobe_t;

int register_jprobe(jprobe_t* jp);
void unregister_jprobe(jprobe_t* jp);

#endif /* KPROBES_H */

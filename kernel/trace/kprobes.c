#include "kprobes.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

static kprobe_t* kprobe_list = NULL;
static spinlock_t kprobe_lock = SPINLOCK_INIT;

void kprobes_init(void) {
    DEBUG_INFO("Kprobes (kernel probes) initialized");
}

int register_kprobe(kprobe_t* kp) {
    if (!kp || !kp->addr) return -1;
    
    spinlock_lock(&kprobe_lock);
    kp->active = true;
    kp->next = kprobe_list;
    kprobe_list = kp;
    spinlock_unlock(&kprobe_lock);
    
    DEBUG_INFO("Kprobe registered at 0x%p", kp->addr);
    return 0;
}

void unregister_kprobe(kprobe_t* kp) {
    if (!kp) return;
    
    spinlock_lock(&kprobe_lock);
    
    kprobe_t** prev = &kprobe_list;
    kprobe_t* current = kprobe_list;
    
    while (current) {
        if (current == kp) {
            *prev = current->next;
            kp->active = false;
            spinlock_unlock(&kprobe_lock);
            DEBUG_INFO("Kprobe unregistered");
            return;
        }
        prev = &current->next;
        current = current->next;
    }
    
    spinlock_unlock(&kprobe_lock);
}

int register_jprobe(jprobe_t* jp) {
    if (!jp) return -1;
    return register_kprobe(&jp->kp);
}

void unregister_jprobe(jprobe_t* jp) {
    if (!jp) return;
    unregister_kprobe(&jp->kp);
}

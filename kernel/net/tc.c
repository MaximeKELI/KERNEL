#include "tc.h"
#include "net.h"
#include "skbuff.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"

#define MAX_TC_CLASSES 256

/* Traffic control class */
typedef struct tc_class {
    u32 classid;
    u64 rate;        /* Bits per second */
    u64 ceil;         /* Maximum rate */
    u64 burst;        /* Burst size */
    u64 packets;
    u64 bytes;
    struct tc_class* next;
} tc_class_t;

/* Traffic control qdisc */
typedef struct tc_qdisc {
    char name[32];
    u32 handle;
    u32 parent;
    tc_class_t* classes;
    struct tc_qdisc* next;
} tc_qdisc_t;

static tc_qdisc_t* tc_qdiscs = NULL;
static spinlock_t tc_lock = SPINLOCK_INIT;

void tc_init(void) {
    tc_qdiscs = NULL;
    printk("[TC] Traffic Control initialized\n");
}

int tc_add_qdisc(const char* name, u32 handle, u32 parent) {
    if (!name) {
        return -1;
    }
    
    tc_qdisc_t* qdisc = (tc_qdisc_t*)kzalloc(sizeof(tc_qdisc_t));
    if (!qdisc) {
        return -1;
    }
    
    strncpy(qdisc->name, name, sizeof(qdisc->name) - 1);
    qdisc->handle = handle;
    qdisc->parent = parent;
    qdisc->classes = NULL;
    
    spinlock_lock(&tc_lock);
    qdisc->next = tc_qdiscs;
    tc_qdiscs = qdisc;
    spinlock_unlock(&tc_lock);
    
    DEBUG_INFO("TC qdisc added: %s, handle=%u", name, handle);
    return 0;
}

int tc_add_class(u32 qdisc_handle, u32 classid, u64 rate, u64 ceil, u64 burst) {
    spinlock_lock(&tc_lock);
    
    tc_qdisc_t* qdisc = tc_qdiscs;
    while (qdisc) {
        if (qdisc->handle == qdisc_handle) {
            tc_class_t* cls = (tc_class_t*)kzalloc(sizeof(tc_class_t));
            if (cls) {
                cls->classid = classid;
                cls->rate = rate;
                cls->ceil = ceil;
                cls->burst = burst;
                cls->packets = 0;
                cls->bytes = 0;
                cls->next = qdisc->classes;
                qdisc->classes = cls;
                
                spinlock_unlock(&tc_lock);
                DEBUG_INFO("TC class added: classid=%u, rate=%u", classid, (u32)rate);
                return 0;
            }
            break;
        }
        qdisc = qdisc->next;
    }
    
    spinlock_unlock(&tc_lock);
    return -1;
}

int tc_enqueue(sk_buff_t* skb, u32 qdisc_handle, u32 classid) {
    if (!skb) {
        return -1;
    }
    
    spinlock_lock(&tc_lock);
    
    tc_qdisc_t* qdisc = tc_qdiscs;
    while (qdisc) {
        if (qdisc->handle == qdisc_handle) {
            tc_class_t* cls = qdisc->classes;
            while (cls) {
                if (cls->classid == classid) {
                    /* Check rate limiting */
                    u64 current_rate = (cls->bytes * 8) / 1000; /* Simplified */
                    if (current_rate < cls->rate) {
                        cls->packets++;
                        cls->bytes += skb->len;
                        spinlock_unlock(&tc_lock);
                        return 0; /* Accept */
                    } else {
                        /* Rate limit exceeded */
                        spinlock_unlock(&tc_lock);
                        return -1; /* Drop */
                    }
                }
                cls = cls->next;
            }
            break;
        }
        qdisc = qdisc->next;
    }
    
    spinlock_unlock(&tc_lock);
    return 0; /* No qdisc, accept */
}

int tc_del_qdisc(u32 handle) {
    spinlock_lock(&tc_lock);
    
    tc_qdisc_t* prev = NULL;
    tc_qdisc_t* qdisc = tc_qdiscs;
    
    while (qdisc) {
        if (qdisc->handle == handle) {
            /* Free classes */
            tc_class_t* cls = qdisc->classes;
            while (cls) {
                tc_class_t* next = cls->next;
                kfree(cls);
                cls = next;
            }
            
            if (prev) {
                prev->next = qdisc->next;
            } else {
                tc_qdiscs = qdisc->next;
            }
            kfree(qdisc);
            
            spinlock_unlock(&tc_lock);
            return 0;
        }
        prev = qdisc;
        qdisc = qdisc->next;
    }
    
    spinlock_unlock(&tc_lock);
    return -1;
}

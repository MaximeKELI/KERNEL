#include "packet_sched.h"
#include "net.h"
#include "skbuff.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "types.h"
#include "string.h"

#define MAX_SCHED_QUEUES 64
#define SCHED_QUEUE_SIZE 1024

/* Packet scheduler queue */
typedef struct sched_queue {
    u32 id;
    char name[32];
    sk_buff_t* packets[SCHED_QUEUE_SIZE];
    u32 head;
    u32 tail;
    u32 count;
    u64 bytes;
    u64 packets_processed;
    spinlock_t lock;
    struct sched_queue* next;
} sched_queue_t;

static sched_queue_t* sched_queues = NULL;
static u32 sched_queue_count = 0;
static spinlock_t sched_global_lock = SPINLOCK_INIT;

void packet_sched_init(void) {
    sched_queues = NULL;
    sched_queue_count = 0;
    printk("[Packet Sched] Packet scheduler initialized\n");
}

sched_queue_t* packet_sched_create_queue(const char* name) {
    if (!name || sched_queue_count >= MAX_SCHED_QUEUES) {
        return NULL;
    }
    
    sched_queue_t* queue = (sched_queue_t*)kzalloc(sizeof(sched_queue_t));
    if (!queue) {
        return NULL;
    }
    
    queue->id = sched_queue_count++;
    strncpy(queue->name, name, sizeof(queue->name) - 1);
    queue->head = 0;
    queue->tail = 0;
    queue->count = 0;
    queue->bytes = 0;
    queue->packets_processed = 0;
    spinlock_init(&queue->lock);
    
    spinlock_lock(&sched_global_lock);
    queue->next = sched_queues;
    sched_queues = queue;
    spinlock_unlock(&sched_global_lock);
    
    DEBUG_INFO("Packet scheduler queue created: %s", name);
    return queue;
}

int packet_sched_enqueue(sched_queue_t* queue, sk_buff_t* skb) {
    if (!queue || !skb) {
        return -1;
    }
    
    spinlock_lock(&queue->lock);
    
    if (queue->count >= SCHED_QUEUE_SIZE) {
        spinlock_unlock(&queue->lock);
        DEBUG_ERROR("Packet scheduler queue full");
        return -1;
    }
    
    queue->packets[queue->tail] = skb;
    queue->tail = (queue->tail + 1) % SCHED_QUEUE_SIZE;
    queue->count++;
    queue->bytes += skb->len;
    
    spinlock_unlock(&queue->lock);
    
    return 0;
}

sk_buff_t* packet_sched_dequeue(sched_queue_t* queue) {
    if (!queue) {
        return NULL;
    }
    
    spinlock_lock(&queue->lock);
    
    if (queue->count == 0) {
        spinlock_unlock(&queue->lock);
        return NULL;
    }
    
    sk_buff_t* skb = queue->packets[queue->head];
    queue->head = (queue->head + 1) % SCHED_QUEUE_SIZE;
    queue->count--;
    queue->bytes -= skb->len;
    queue->packets_processed++;
    
    spinlock_unlock(&queue->lock);
    
    return skb;
}

void packet_sched_destroy_queue(sched_queue_t* queue) {
    if (!queue) {
        return;
    }
    
    /* Free all packets */
    sk_buff_t* skb;
    while ((skb = packet_sched_dequeue(queue)) != NULL) {
        skb_free(skb);
    }
    
    spinlock_lock(&sched_global_lock);
    
    if (sched_queues == queue) {
        sched_queues = queue->next;
    } else {
        sched_queue_t* q = sched_queues;
        while (q && q->next != queue) {
            q = q->next;
        }
        if (q) {
            q->next = queue->next;
        }
    }
    
    sched_queue_count--;
    spinlock_unlock(&sched_global_lock);
    
    kfree(queue);
}

void packet_sched_get_stats(sched_queue_t* queue, u64* packets, u64* bytes, u64* processed) {
    if (!queue) {
        return;
    }
    
    spinlock_lock(&queue->lock);
    if (packets) *packets = queue->count;
    if (bytes) *bytes = queue->bytes;
    if (processed) *processed = queue->packets_processed;
    spinlock_unlock(&queue->lock);
}

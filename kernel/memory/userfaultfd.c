#include "userfaultfd.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "vmm.h"

#define MAX_USERFAULTFD_FDS 32

typedef struct userfaultfd_range {
    void* start;
    void* end;
    u64 mode;
    struct userfaultfd_range* next;
} userfaultfd_range_t;

typedef struct userfaultfd_entry {
    i32 fd;
    bool nonblock;
    bool active;
    u64 api;
    u64 features;
    userfaultfd_range_t* ranges;
    uffd_msg_t event_queue[64];
    u32 queue_head;
    u32 queue_tail;
    u32 queue_count;
    spinlock_t lock;
} userfaultfd_entry_t;

static userfaultfd_entry_t userfaultfd_table[MAX_USERFAULTFD_FDS];
static i32 next_fd = 0;
static spinlock_t userfaultfd_global_lock = SPINLOCK_INIT;

static userfaultfd_entry_t* userfaultfd_get_entry(i32 fd) {
    if (fd < 0 || fd >= MAX_USERFAULTFD_FDS || !userfaultfd_table[fd].active) {
        return NULL;
    }
    return &userfaultfd_table[fd];
}

i32 userfaultfd(i32 flags) {
    spinlock_lock(&userfaultfd_global_lock);
    
    for (u32 i = 0; i < MAX_USERFAULTFD_FDS; i++) {
        if (!userfaultfd_table[i].active) {
            userfaultfd_entry_t* entry = &userfaultfd_table[i];
            entry->fd = next_fd++;
            entry->nonblock = (flags & UFFD_NONBLOCK) != 0;
            entry->active = true;
            entry->api = 0;
            entry->features = 0;
            entry->ranges = NULL;
            entry->queue_head = 0;
            entry->queue_tail = 0;
            entry->queue_count = 0;
            spinlock_init(&entry->lock);
            
            spinlock_unlock(&userfaultfd_global_lock);
            DEBUG_INFO("Userfaultfd created: fd=%d", entry->fd);
            return entry->fd;
        }
    }
    
    spinlock_unlock(&userfaultfd_global_lock);
    DEBUG_ERROR("%s", "Maximum userfaultfd instances reached");
    return -1;
}

ssize_t userfaultfd_read(i32 fd, uffd_msg_t* msg) {
    VALIDATE_PTR_RET(msg, -1);
    
    userfaultfd_entry_t* entry = userfaultfd_get_entry(fd);
    if (!entry) {
        return -1;
    }
    
    spinlock_lock(&entry->lock);
    
    if (entry->queue_count == 0) {
        spinlock_unlock(&entry->lock);
        if (entry->nonblock) {
            return -1; /* Would set EAGAIN */
        }
        return 0; /* Block until event */
    }
    
    *msg = entry->event_queue[entry->queue_head];
    entry->queue_head = (entry->queue_head + 1) % 64;
    entry->queue_count--;
    
    spinlock_unlock(&entry->lock);
    return sizeof(uffd_msg_t);
}

ssize_t userfaultfd_write(i32 fd, const void* buf, size_t count) {
    VALIDATE_PTR_RET(buf, -1);
    
    userfaultfd_entry_t* entry = userfaultfd_get_entry(fd);
    if (!entry) {
        return -1;
    }
    
    /* Write is used to wake up faulting thread */
    /* Would implement wake mechanism here */
    
    (void)count;
    return 0;
}

/* Internal function to handle page fault */
int userfaultfd_handle_pagefault(void* addr, u32 flags) {
    for (u32 i = 0; i < MAX_USERFAULTFD_FDS; i++) {
        if (!userfaultfd_table[i].active) continue;
        
        userfaultfd_entry_t* entry = &userfaultfd_table[i];
        spinlock_lock(&entry->lock);
        
        /* Check if address is in any registered range */
        userfaultfd_range_t* range = entry->ranges;
        bool in_range = false;
        while (range) {
            if (addr >= range->start && addr < range->end) {
                in_range = true;
                break;
            }
            range = range->next;
        }
        
        if (in_range && entry->queue_count < 64) {
            uffd_msg_t* msg = &entry->event_queue[entry->queue_tail];
            msg->event = 0; /* UFFD_EVENT_PAGEFAULT */
            msg->arg.pagefault.address = (u64)addr;
            msg->arg.pagefault.flags = flags;
            msg->arg.pagefault.ptid = 0; /* Would set to thread ID */
            
            entry->queue_tail = (entry->queue_tail + 1) % 64;
            entry->queue_count++;
        }
        
        spinlock_unlock(&entry->lock);
    }
    
    return 0;
}

#include "signalfd.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "signal.h"

#define MAX_SIGNALFD_FDS 64

typedef struct signalfd_entry {
    i32 fd;
    sigset_t mask;
    bool nonblock;
    bool active;
    signalfd_siginfo_t pending_signals[32];
    u32 signal_count;
    u32 signal_read;
    spinlock_t lock;
} signalfd_entry_t;

static signalfd_entry_t signalfd_table[MAX_SIGNALFD_FDS];
static i32 next_fd = 0;
static spinlock_t signalfd_global_lock = SPINLOCK_INIT;

i32 signalfd(i32 fd, const sigset_t* mask, i32 flags) {
    VALIDATE_PTR_RET(mask, -1);
    
    spinlock_lock(&signalfd_global_lock);
    
    if (fd >= 0) {
        /* Modify existing signalfd */
        if (fd < MAX_SIGNALFD_FDS && signalfd_table[fd].active) {
            signalfd_entry_t* entry = &signalfd_table[fd];
            spinlock_lock(&entry->lock);
            entry->mask = *mask;
            spinlock_unlock(&entry->lock);
            spinlock_unlock(&signalfd_global_lock);
            DEBUG_INFO("Signalfd modified: fd=%d", fd);
            return fd;
        }
        spinlock_unlock(&signalfd_global_lock);
        return -1;
    }
    
    /* Create new signalfd */
    for (u32 i = 0; i < MAX_SIGNALFD_FDS; i++) {
        if (!signalfd_table[i].active) {
            signalfd_entry_t* entry = &signalfd_table[i];
            entry->fd = next_fd++;
            entry->mask = *mask;
            entry->nonblock = (flags & SFD_NONBLOCK) != 0;
            entry->active = true;
            entry->signal_count = 0;
            entry->signal_read = 0;
            spinlock_init(&entry->lock);
            
            spinlock_unlock(&signalfd_global_lock);
            DEBUG_INFO("Signalfd created: fd=%d", entry->fd);
            return entry->fd;
        }
    }
    
    spinlock_unlock(&signalfd_global_lock);
    DEBUG_ERROR("%s", "Maximum signalfd instances reached");
    return -1;
}

ssize_t signalfd_read(i32 fd, signalfd_siginfo_t* info) {
    VALIDATE_PTR_RET(info, -1);
    
    if (fd < 0 || fd >= MAX_SIGNALFD_FDS || !signalfd_table[fd].active) {
        return -1;
    }
    
    signalfd_entry_t* entry = &signalfd_table[fd];
    spinlock_lock(&entry->lock);
    
    if (entry->signal_read >= entry->signal_count) {
        spinlock_unlock(&entry->lock);
        if (entry->nonblock) {
            return -1; /* Would set EAGAIN */
        }
        return 0; /* Block until signal */
    }
    
    *info = entry->pending_signals[entry->signal_read++];
    
    spinlock_unlock(&entry->lock);
    return sizeof(signalfd_siginfo_t);
}

/* Internal function to deliver signal to signalfd */
void signalfd_deliver_signal(i32 signo, u32 pid, u32 uid) {
    for (u32 i = 0; i < MAX_SIGNALFD_FDS; i++) {
        if (!signalfd_table[i].active) continue;
        
        signalfd_entry_t* entry = &signalfd_table[i];
        spinlock_lock(&entry->lock);
        
        /* Check if signal is in mask */
        u32 sig_word = signo / 64;
        u32 sig_bit = signo % 64;
        if (sig_word < (NSIG + 63) / 64 && (entry->mask.sig[sig_word] & (1ULL << sig_bit))) {
            if (entry->signal_count < 32) {
                signalfd_siginfo_t* info = &entry->pending_signals[entry->signal_count++];
                memset(info, 0, sizeof(signalfd_siginfo_t));
                info->ssi_signo = signo;
                info->ssi_pid = pid;
                info->ssi_uid = uid;
            }
        }
        
        spinlock_unlock(&entry->lock);
    }
}

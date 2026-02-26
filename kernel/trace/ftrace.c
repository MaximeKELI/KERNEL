#include "ftrace.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "drivers/timer.h"

#define FTRACE_BUFFER_SIZE 4096

static ftrace_entry_t ftrace_buffer[FTRACE_BUFFER_SIZE];
static u32 ftrace_head = 0;
static u32 ftrace_count = 0;
static bool ftrace_enabled_flag = false;
static spinlock_t ftrace_lock = SPINLOCK_INIT;

void ftrace_init(void) {
    memset(ftrace_buffer, 0, sizeof(ftrace_buffer));
    DEBUG_INFO("Ftrace (function tracer) initialized");
}

int ftrace_enable(void) {
    spinlock_lock(&ftrace_lock);
    ftrace_enabled_flag = true;
    spinlock_unlock(&ftrace_lock);
    DEBUG_INFO("Ftrace enabled");
    return 0;
}

int ftrace_disable(void) {
    spinlock_lock(&ftrace_lock);
    ftrace_enabled_flag = false;
    spinlock_unlock(&ftrace_lock);
    DEBUG_INFO("Ftrace disabled");
    return 0;
}

void ftrace_function_entry(void* func) {
    if (!ftrace_enabled_flag) return;
    
    spinlock_lock(&ftrace_lock);
    
    ftrace_entry_t* entry = &ftrace_buffer[ftrace_head];
    entry->timestamp = timer_get_ticks();
    entry->ip = (u64)func;
    entry->parent_ip = 0; /* Would get from stack */
    strncpy(entry->func_name, "unknown", sizeof(entry->func_name) - 1);
    
    ftrace_head = (ftrace_head + 1) % FTRACE_BUFFER_SIZE;
    if (ftrace_count < FTRACE_BUFFER_SIZE) {
        ftrace_count++;
    }
    
    spinlock_unlock(&ftrace_lock);
}

void ftrace_function_exit(void* func) {
    (void)func;
    /* Would trace exit */
}

int ftrace_read(ftrace_entry_t* entries, u32 count) {
    if (!entries || count == 0) return 0;
    
    spinlock_lock(&ftrace_lock);
    
    u32 to_read = (count < ftrace_count) ? count : ftrace_count;
    u32 start = (ftrace_head >= to_read) ? (ftrace_head - to_read) : 
                (FTRACE_BUFFER_SIZE - (to_read - ftrace_head));
    
    for (u32 i = 0; i < to_read; i++) {
        u32 idx = (start + i) % FTRACE_BUFFER_SIZE;
        entries[i] = ftrace_buffer[idx];
    }
    
    spinlock_unlock(&ftrace_lock);
    return to_read;
}

void ftrace_clear(void) {
    spinlock_lock(&ftrace_lock);
    ftrace_head = ftrace_count = 0;
    memset(ftrace_buffer, 0, sizeof(ftrace_buffer));
    spinlock_unlock(&ftrace_lock);
}

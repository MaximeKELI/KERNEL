#include "log.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "spinlock.h"
#include "drivers/timer.h"
#include <stdarg.h>

#define MAX_LOG_ENTRIES (LOG_BUFFER_SIZE / sizeof(log_entry_t))

static log_entry_t log_buffer[MAX_LOG_ENTRIES];
static u32 log_head = 0;
static u32 log_tail = 0;
static u32 log_count = 0;
static spinlock_t log_lock = SPINLOCK_INIT;

static const char* level_names[] = {
    "EMERG", "ALERT", "CRIT", "ERR", "WARNING", "NOTICE", "INFO", "DEBUG"
};

void log_init(void) {
    memset(log_buffer, 0, sizeof(log_buffer));
    printk("Logging system initialized\n");
}

void klog(u8 level, u8 facility, const char* format, ...) {
    if (level > LOG_DEBUG) return;
    
    va_list args;
    va_start(args, format);
    
    spinlock_lock(&log_lock);
    
    log_entry_t* entry = &log_buffer[log_head];
    entry->timestamp = timer_get_ticks();
    entry->level = level;
    entry->facility = facility;
    
    /* Format message */
    char* msg = entry->message;
    int written = 0;
    
    /* Add level prefix */
    written += snprintf(msg + written, sizeof(entry->message) - written,
                       "[%s] ", level_names[level]);
    
    /* Format user message */
    written += vsnprintf(msg + written, sizeof(entry->message) - written,
                        format, args);
    
    entry->message[sizeof(entry->message) - 1] = '\0';
    
    /* Also print to console */
    printk("%s\n", entry->message);
    
    log_head = (log_head + 1) % MAX_LOG_ENTRIES;
    if (log_count < MAX_LOG_ENTRIES) {
        log_count++;
    } else {
        log_tail = (log_tail + 1) % MAX_LOG_ENTRIES;
    }
    
    spinlock_unlock(&log_lock);
    
    va_end(args);
}

int log_read(log_entry_t* entries, u32 count) {
    if (!entries || count == 0) return 0;
    
    spinlock_lock(&log_lock);
    
    u32 to_read = (count < log_count) ? count : log_count;
    u32 idx = log_tail;
    
    for (u32 i = 0; i < to_read; i++) {
        entries[i] = log_buffer[idx];
        idx = (idx + 1) % MAX_LOG_ENTRIES;
    }
    
    spinlock_unlock(&log_lock);
    
    return to_read;
}

void log_clear(void) {
    spinlock_lock(&log_lock);
    log_head = log_tail = log_count = 0;
    memset(log_buffer, 0, sizeof(log_buffer));
    spinlock_unlock(&log_lock);
}

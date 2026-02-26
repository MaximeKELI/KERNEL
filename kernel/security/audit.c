#include "audit.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "process.h"
#include "drivers/timer.h"

#define MAX_AUDIT_RECORDS 1024

static audit_record_t* audit_log_head = NULL;
static audit_record_t* audit_log_tail = NULL;
static u32 audit_record_count = 0;
static bool audit_enabled_flag = true;
static spinlock_t audit_lock = SPINLOCK_INIT;

void audit_init(void) {
    DEBUG_INFO("Audit system initialized");
}

void audit_log(u32 type, const char* message) {
    if (!audit_enabled_flag) return;
    
    audit_record_t* record = (audit_record_t*)kzalloc(sizeof(audit_record_t));
    if (!record) {
        DEBUG_ERROR("Failed to allocate audit record");
        return;
    }
    
    process_t* proc = process_current();
    
    record->timestamp = timer_get_ticks();
    record->type = type;
    record->pid = proc ? proc->pid : 0;
    record->uid = proc ? proc->uid : 0;
    strncpy(record->message, message, sizeof(record->message) - 1);
    
    spinlock_lock(&audit_lock);
    
    if (!audit_log_head) {
        audit_log_head = audit_log_tail = record;
    } else {
        audit_log_tail->next = record;
        audit_log_tail = record;
    }
    
    audit_record_count++;
    
    /* Limit log size */
    if (audit_record_count > MAX_AUDIT_RECORDS) {
        audit_record_t* old = audit_log_head;
        audit_log_head = audit_log_head->next;
        kfree(old);
        audit_record_count--;
    }
    
    spinlock_unlock(&audit_lock);
    
    DEBUG_INFO("Audit: type=%u, pid=%u, msg=%s", type, (u32)record->pid, message);
}

int audit_read(audit_record_t* records, u32 count) {
    if (!records || count == 0) return 0;
    
    spinlock_lock(&audit_lock);
    
    u32 copied = 0;
    audit_record_t* current = audit_log_head;
    
    while (current && copied < count) {
        records[copied] = *current;
        records[copied].next = NULL;
        current = current->next;
        copied++;
    }
    
    spinlock_unlock(&audit_lock);
    
    return copied;
}

void audit_clear(void) {
    spinlock_lock(&audit_lock);
    
    audit_record_t* current = audit_log_head;
    while (current) {
        audit_record_t* next = current->next;
        kfree(current);
        current = next;
    }
    
    audit_log_head = audit_log_tail = NULL;
    audit_record_count = 0;
    
    spinlock_unlock(&audit_lock);
}

int audit_enable(bool enable) {
    audit_enabled_flag = enable;
    return 0;
}

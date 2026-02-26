#ifndef AUDIT_H
#define AUDIT_H

#include "types.h"

/* Audit event types */
#define AUDIT_SYSCALL  1300
#define AUDIT_PATH     1302
#define AUDIT_IPC      1303
#define AUDIT_SOCKET   1306
#define AUDIT_CONFIG   1309
#define AUDIT_LOGIN    1112
#define AUDIT_USER     1100

/* Audit record */
typedef struct audit_record {
    u64 timestamp;
    u32 type;
    u64 pid;
    u64 uid;
    char message[512];
    struct audit_record* next;
} audit_record_t;

/* Initialize audit system */
void audit_init(void);

/* Log audit event */
void audit_log(u32 type, const char* message);

/* Read audit log */
int audit_read(audit_record_t* records, u32 count);

/* Clear audit log */
void audit_clear(void);

/* Enable/disable audit */
int audit_enable(bool enable);

#endif /* AUDIT_H */

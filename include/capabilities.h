#ifndef CAPABILITIES_H
#define CAPABILITIES_H

#include "types.h"

/* Capability numbers */
#define CAP_CHOWN           0
#define CAP_DAC_OVERRIDE    1
#define CAP_DAC_READ_SEARCH 2
#define CAP_FOWNER          3
#define CAP_FSETID          4
#define CAP_KILL            5
#define CAP_SETGID          6
#define CAP_SETUID          7
#define CAP_SETPCAP         8
#define CAP_LINUX_IMMUTABLE 9
#define CAP_NET_BIND_SERVICE 10
#define CAP_NET_BROADCAST   11
#define CAP_NET_ADMIN       12
#define CAP_NET_RAW         13
#define CAP_IPC_LOCK        14
#define CAP_IPC_OWNER       15
#define CAP_SYS_MODULE      16
#define CAP_SYS_RAWIO       17
#define CAP_SYS_CHROOT      18
#define CAP_SYS_PTRACE      19
#define CAP_SYS_PACCT       20
#define CAP_SYS_ADMIN       21
#define CAP_SYS_BOOT        22
#define CAP_SYS_NICE        23
#define CAP_SYS_RESOURCE    24
#define CAP_SYS_TIME        25
#define CAP_SYS_TTY_CONFIG  26
#define CAP_MKNOD           27
#define CAP_LEASE           28
#define CAP_AUDIT_WRITE      29
#define CAP_AUDIT_CONTROL   30
#define CAP_SETFCAP         31

#define CAP_LAST_CAP 31

/* Capability set */
typedef struct {
    u64 effective;
    u64 permitted;
    u64 inheritable;
} cap_t;

/* Initialize capabilities */
void capabilities_init(void);

/* Check capability */
bool capable(int cap);

/* Set capability */
int cap_set(int cap, bool value);

/* Get capabilities */
int cap_get(cap_t* caps);

/* Set capabilities */
int cap_set_all(cap_t* caps);

#endif /* CAPABILITIES_H */

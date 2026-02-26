#ifndef SYSV_IPC_H
#define SYSV_IPC_H

#include "types.h"
#include "ipc.h"

/* System V IPC keys */
#define IPC_PRIVATE 0

/* Message queue */
typedef struct msqid_ds {
    u64 key;
    u32 msqid;
    u32 msg_count;
    size_t msg_bytes;
    void* messages;
    spinlock_t lock;
} msqid_ds_t;

/* Semaphore array */
typedef struct semid_ds {
    u64 key;
    u32 semid;
    u16 nsems;
    i16* sem_array;
    spinlock_t lock;
} semid_ds_t;

/* Initialize System V IPC */
void sysv_ipc_init(void);

/* Message queue operations */
int msgget(u64 key, int flags);
int msgsnd(int msqid, const void* msgp, size_t msgsz, int flags);
ssize_t msgrcv(int msqid, void* msgp, size_t msgsz, long msgtyp, int flags);
int msgctl(int msqid, int cmd, void* buf);

/* Semaphore operations */
int semget(u64 key, int nsems, int flags);
int semop(int semid, void* sops, size_t nsops);
int semctl(int semid, int semnum, int cmd, void* arg);

#endif /* SYSV_IPC_H */

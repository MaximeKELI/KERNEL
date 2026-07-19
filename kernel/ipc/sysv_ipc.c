#include "sysv_ipc.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"

#define MAX_MSG_QUEUES 128
#define MAX_SEM_ARRAYS 128

static msqid_ds_t msg_queues[MAX_MSG_QUEUES];
static semid_ds_t sem_arrays[MAX_SEM_ARRAYS];
static u32 next_msgid = 1;
static u32 next_semid = 1;

void sysv_ipc_init(void) {
    memset(msg_queues, 0, sizeof(msg_queues));
    memset(sem_arrays, 0, sizeof(sem_arrays));
    DEBUG_INFO("System V IPC initialized");
}

int msgget(u64 key, int flags) {
    (void)flags;
    
    /* Find existing queue */
    for (u32 i = 0; i < MAX_MSG_QUEUES; i++) {
        if (msg_queues[i].key == key && msg_queues[i].msqid != 0) {
            return msg_queues[i].msqid;
        }
    }
    
    /* Create new queue */
    for (u32 i = 0; i < MAX_MSG_QUEUES; i++) {
        if (msg_queues[i].msqid == 0) {
            msg_queues[i].key = key;
            msg_queues[i].msqid = next_msgid++;
            msg_queues[i].msg_count = 0;
            msg_queues[i].msg_bytes = 0;
            spinlock_init(&msg_queues[i].lock);
            return msg_queues[i].msqid;
        }
    }
    
    return -1;
}

int msgsnd(int msqid, const void* msgp, size_t msgsz, int flags) {
    (void)flags;
    
    for (u32 i = 0; i < MAX_MSG_QUEUES; i++) {
        if (msg_queues[i].msqid == msqid) {
            spinlock_lock(&msg_queues[i].lock);
            /* Would add message to queue */
            msg_queues[i].msg_count++;
            msg_queues[i].msg_bytes += msgsz;
            spinlock_unlock(&msg_queues[i].lock);
            return 0;
        }
    }
    
    return -1;
}

ssize_t msgrcv(int msqid, void* msgp, size_t msgsz, long msgtyp, int flags) {
    (void)msgtyp;
    (void)flags;
    
    for (u32 i = 0; i < MAX_MSG_QUEUES; i++) {
        if (msg_queues[i].msqid == msqid) {
            spinlock_lock(&msg_queues[i].lock);
            if (msg_queues[i].msg_count > 0) {
                /* Would copy message */
                msg_queues[i].msg_count--;
                msg_queues[i].msg_bytes -= msgsz;
                spinlock_unlock(&msg_queues[i].lock);
                return msgsz;
            }
            spinlock_unlock(&msg_queues[i].lock);
            return 0;
        }
    }
    
    return -1;
}

int msgctl(int msqid, int cmd, void* buf) {
    (void)cmd;
    (void)buf;
    (void)msqid;
    return 0;
}

int semget(u64 key, int nsems, int flags) {
    (void)flags;
    
    for (u32 i = 0; i < MAX_SEM_ARRAYS; i++) {
        if (sem_arrays[i].key == key && sem_arrays[i].semid != 0) {
            return sem_arrays[i].semid;
        }
    }
    
    for (u32 i = 0; i < MAX_SEM_ARRAYS; i++) {
        if (sem_arrays[i].semid == 0) {
            sem_arrays[i].key = key;
            sem_arrays[i].semid = next_semid++;
            sem_arrays[i].nsems = nsems;
            sem_arrays[i].sem_array = (i16*)kzalloc(nsems * sizeof(i16));
            spinlock_init(&sem_arrays[i].lock);
            return sem_arrays[i].semid;
        }
    }
    
    return -1;
}

int semop(int semid, void* sops, size_t nsops) {
    (void)sops;
    (void)nsops;
    
    for (u32 i = 0; i < MAX_SEM_ARRAYS; i++) {
        if (sem_arrays[i].semid == semid) {
            spinlock_lock(&sem_arrays[i].lock);
            /* Would perform semaphore operations */
            spinlock_unlock(&sem_arrays[i].lock);
            return 0;
        }
    }
    
    return -1;
}

int semctl(int semid, int semnum, int cmd, void* arg) {
    (void)semnum;
    (void)cmd;
    (void)arg;
    
    for (u32 i = 0; i < MAX_SEM_ARRAYS; i++) {
        if (sem_arrays[i].semid == semid) {
            return 0;
        }
    }
    
    return -1;
}

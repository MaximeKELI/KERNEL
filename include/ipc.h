#ifndef IPC_H
#define IPC_H

#include "types.h"

/* IPC types */
#define IPC_PIPE       1
#define IPC_SHM        2
#define IPC_SEM        3
#define IPC_MSG        4

/* Pipe */
typedef struct pipe {
    u8* buffer;
    size_t size;
    size_t read_pos;
    size_t write_pos;
    u32 readers;
    u32 writers;
    spinlock_t lock;
} pipe_t;

/* Shared memory */
typedef struct shm {
    void* addr;
    size_t size;
    u32 refcount;
    u64 key;
} shm_t;

/* Semaphore */
typedef struct sem {
    i32 value;
    u32 waiters;
    spinlock_t lock;
} sem_t;

/* Initialize IPC */
void ipc_init(void);

/* Pipe operations */
int pipe_create(pipe_t** read_end, pipe_t** write_end);
ssize_t pipe_read(pipe_t* pipe, void* buf, size_t count);
ssize_t pipe_write(pipe_t* pipe, const void* buf, size_t count);
void pipe_close(pipe_t* pipe);

/* Shared memory */
int shm_create(u64 key, size_t size, void** addr);
int shm_attach(u64 key, void** addr);
int shm_detach(void* addr);
int shm_destroy(u64 key);

/* Semaphore */
int sem_create(u64 key, i32 initial_value);
int sem_wait(u64 key);
int sem_post(u64 key);
int sem_destroy(u64 key);

#endif /* IPC_H */

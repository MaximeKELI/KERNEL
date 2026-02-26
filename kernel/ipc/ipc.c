#include "ipc.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "validate.h"
#include "overflow.h"

#define MAX_PIPES 256
#define MAX_SHM 64
#define MAX_SEM 64

static pipe_t pipes[MAX_PIPES];
static shm_t shm_segments[MAX_SHM];
static sem_t semaphores[MAX_SEM];
static u32 next_pipe_id = 0;
static u32 next_shm_id = 0;
static u32 next_sem_id = 0;

void ipc_init(void) {
    memset(pipes, 0, sizeof(pipes));
    memset(shm_segments, 0, sizeof(shm_segments));
    memset(semaphores, 0, sizeof(semaphores));
    DEBUG_INFO("IPC system initialized");
}

int pipe_create(pipe_t** read_end, pipe_t** write_end) {
    VALIDATE_PTR(read_end);
    VALIDATE_PTR(write_end);
    
    if (next_pipe_id >= MAX_PIPES) {
        DEBUG_ERROR("Maximum pipes reached");
        return -1;
    }
    
    pipe_t* pipe = &pipes[next_pipe_id++];
    pipe->size = PAGE_SIZE;
    pipe->buffer = (u8*)kmalloc(pipe->size);
    if (!pipe->buffer) {
        DEBUG_ERROR("Failed to allocate pipe buffer");
        return -1;
    }
    
    pipe->read_pos = 0;
    pipe->write_pos = 0;
    pipe->readers = 1;
    pipe->writers = 1;
    spinlock_init(&pipe->lock);
    
    *read_end = pipe;
    *write_end = pipe;
    
    return 0;
}

ssize_t pipe_read(pipe_t* pipe, void* buf, size_t count) {
    VALIDATE_PTR_RET(pipe, -1);
    VALIDATE_PTR_RET(buf, -1);
    VALIDATE_RANGE(count, 0, 1024 * 1024);
    
    spinlock_lock(&pipe->lock);
    
    size_t available = (pipe->write_pos >= pipe->read_pos) ?
        (pipe->write_pos - pipe->read_pos) :
        (pipe->size - pipe->read_pos + pipe->write_pos);
    
    if (available == 0) {
        spinlock_unlock(&pipe->lock);
        return 0;
    }
    
    size_t to_read = (count < available) ? count : available;
    u8* dest = (u8*)buf;
    
    for (size_t i = 0; i < to_read; i++) {
        dest[i] = pipe->buffer[pipe->read_pos];
        pipe->read_pos = (pipe->read_pos + 1) % pipe->size;
    }
    
    spinlock_unlock(&pipe->lock);
    return to_read;
}

ssize_t pipe_write(pipe_t* pipe, const void* buf, size_t count) {
    VALIDATE_PTR_RET(pipe, -1);
    VALIDATE_PTR_RET(buf, -1);
    VALIDATE_RANGE(count, 0, 1024 * 1024);
    
    spinlock_lock(&pipe->lock);
    
    size_t free_space = (pipe->read_pos > pipe->write_pos) ?
        (pipe->read_pos - pipe->write_pos - 1) :
        (pipe->size - pipe->write_pos + pipe->read_pos - 1);
    
    if (free_space == 0) {
        spinlock_unlock(&pipe->lock);
        return 0;
    }
    
    size_t to_write = (count < free_space) ? count : free_space;
    const u8* src = (const u8*)buf;
    
    for (size_t i = 0; i < to_write; i++) {
        pipe->buffer[pipe->write_pos] = src[i];
        pipe->write_pos = (pipe->write_pos + 1) % pipe->size;
    }
    
    spinlock_unlock(&pipe->lock);
    return to_write;
}

void pipe_close(pipe_t* pipe) {
    VALIDATE_PTR_VOID(pipe);
    
    spinlock_lock(&pipe->lock);
    if (pipe->readers > 0) pipe->readers--;
    if (pipe->writers > 0) pipe->writers--;
    
    if (pipe->readers == 0 && pipe->writers == 0) {
        kfree(pipe->buffer);
        memset(pipe, 0, sizeof(pipe_t));
    }
    spinlock_unlock(&pipe->lock);
}

int shm_create(u64 key, size_t size, void** addr) {
    VALIDATE_PTR(addr);
    VALIDATE_RANGE(size, 1, 1024 * 1024 * 1024); /* Max 1GB */
    
    if (next_shm_id >= MAX_SHM) {
        DEBUG_ERROR("Maximum shared memory segments reached");
        return -1;
    }
    
    /* Check for overflow in page calculation */
    size_t pages_needed;
    size_t total_pages = (size + PAGE_SIZE - 1) / PAGE_SIZE;
    CHECK_ADD_OVERFLOW(total_pages, 0, &pages_needed);
    
    shm_t* shm = &shm_segments[next_shm_id++];
    shm->addr = vmm_alloc_pages(pages_needed);
    if (!shm->addr) {
        DEBUG_ERROR("Failed to allocate shared memory");
        return -1;
    }
    
    shm->size = size;
    shm->key = key;
    shm->refcount = 1;
    
    *addr = shm->addr;
    return 0;
}

int shm_attach(u64 key, void** addr) {
    VALIDATE_PTR(addr);
    
    for (u32 i = 0; i < MAX_SHM; i++) {
        if (shm_segments[i].key == key && shm_segments[i].addr) {
            shm_segments[i].refcount++;
            *addr = shm_segments[i].addr;
            return 0;
        }
    }
    return -1;
}

int shm_detach(void* addr) {
    VALIDATE_PTR(addr);
    
    for (u32 i = 0; i < MAX_SHM; i++) {
        if (shm_segments[i].addr == addr) {
            shm_segments[i].refcount--;
            if (shm_segments[i].refcount == 0) {
                vmm_free_pages(shm_segments[i].addr, 
                              (shm_segments[i].size + PAGE_SIZE - 1) / PAGE_SIZE);
                memset(&shm_segments[i], 0, sizeof(shm_t));
            }
            return 0;
        }
    }
    return -1;
}

int shm_destroy(u64 key) {
    for (u32 i = 0; i < MAX_SHM; i++) {
        if (shm_segments[i].key == key) {
            if (shm_segments[i].refcount > 0) {
                return -1; /* Still in use */
            }
            vmm_free_pages(shm_segments[i].addr,
                          (shm_segments[i].size + PAGE_SIZE - 1) / PAGE_SIZE);
            memset(&shm_segments[i], 0, sizeof(shm_t));
            return 0;
        }
    }
    return -1;
}

int sem_create(u64 key, i32 initial_value) {
    if (next_sem_id >= MAX_SEM) {
        DEBUG_ERROR("Maximum semaphores reached");
        return -1;
    }
    
    sem_t* sem = &semaphores[next_sem_id++];
    sem->value = initial_value;
    sem->waiters = 0;
    spinlock_init(&sem->lock);
    
    return 0;
}

int sem_wait(u64 key) {
    for (u32 i = 0; i < MAX_SEM; i++) {
        if (semaphores[i].key == key) {
            sem_t* sem = &semaphores[i];
            spinlock_lock(&sem->lock);
            
            while (sem->value <= 0) {
                sem->waiters++;
                spinlock_unlock(&sem->lock);
                asm volatile("hlt"); /* Wait */
                spinlock_lock(&sem->lock);
                sem->waiters--;
            }
            
            sem->value--;
            spinlock_unlock(&sem->lock);
            return 0;
        }
    }
    return -1;
}

int sem_post(u64 key) {
    for (u32 i = 0; i < MAX_SEM; i++) {
        if (semaphores[i].key == key) {
            sem_t* sem = &semaphores[i];
            spinlock_lock(&sem->lock);
            sem->value++;
            spinlock_unlock(&sem->lock);
            return 0;
        }
    }
    return -1;
}

int sem_destroy(u64 key) {
    for (u32 i = 0; i < MAX_SEM; i++) {
        if (semaphores[i].key == key) {
            memset(&semaphores[i], 0, sizeof(sem_t));
            return 0;
        }
    }
    return -1;
}

#include "io_sched.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"

static io_scheduler_t schedulers[3];

/* Noop scheduler */
static int noop_add_request(io_scheduler_t* iosched, bio_t* bio) {
    bio_t** queue = (bio_t**)iosched->private_data;
    bio->next = *queue;
    *queue = bio;
    return 0;
}

static bio_t* noop_dispatch_request(io_scheduler_t* iosched) {
    bio_t** queue = (bio_t**)iosched->private_data;
    if (!*queue) return NULL;
    
    bio_t* bio = *queue;
    *queue = bio->next;
    bio->next = NULL;
    return bio;
}

static void noop_init(io_scheduler_t* iosched) {
    iosched->private_data = kzalloc(sizeof(bio_t*));
}

void io_sched_init(void) {
    /* Initialize NOOP scheduler */
    schedulers[0].type = IOSCHED_NOOP;
    strncpy(schedulers[0].name, "noop", sizeof(schedulers[0].name) - 1);
    schedulers[0].name[sizeof(schedulers[0].name) - 1] = '\0';
    schedulers[0].add_request = noop_add_request;
    schedulers[0].dispatch_request = noop_dispatch_request;
    schedulers[0].init = noop_init;
    noop_init(&schedulers[0]);
    
    DEBUG_INFO("I/O schedulers initialized");
}

int io_sched_set(request_queue_t* queue, u32 type) {
    if (type >= 3) return -1;
    
    io_scheduler_t* iosched = &schedulers[type];
    queue->private_data = iosched;
    
    DEBUG_INFO("I/O scheduler set: %s", iosched->name);
    return 0;
}

io_scheduler_t* io_sched_get(request_queue_t* queue) {
    return (io_scheduler_t*)queue->private_data;
}

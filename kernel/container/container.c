#include "container.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "namespace.h"
#include "cgroup.h"
#include "validate.h"

static container_t* container_list_head = NULL;
static u64 next_container_id = 1;
static spinlock_t container_lock = SPINLOCK_INIT;

void container_init(void) {
    DEBUG_INFO("Container runtime initialized");
}

container_t* container_create(const char* name, const char* rootfs) {
    VALIDATE_PTR_RET(name, NULL);
    VALIDATE_PTR_RET(rootfs, NULL);
    VALIDATE_STRING(name, 63);
    
    container_t* container = (container_t*)kzalloc(sizeof(container_t));
    if (!container) {
        DEBUG_ERROR("Failed to allocate container");
        return NULL;
    }
    
    strncpy(container->name, name, sizeof(container->name) - 1);
    container->container_id = next_container_id++;
    container->rootfs = (void*)rootfs; /* Would mount rootfs */
    container->running = false;
    
    /* Create namespaces */
    container->namespaces = namespace_create(CLONE_NEWPID | CLONE_NEWNS | CLONE_NEWNET);
    if (!container->namespaces) {
        kfree(container);
        return NULL;
    }
    
    /* Create cgroup */
    container->cgroup = cgroup_create(container->name, NULL);
    if (!container->cgroup) {
        kfree(container->namespaces);
        kfree(container);
        return NULL;
    }
    
    spinlock_lock(&container_lock);
    container->next = container_list_head;
    container_list_head = container;
    spinlock_unlock(&container_lock);
    
    DEBUG_INFO("Container created: name=%s, id=%u", name, (u32)container->container_id);
    return container;
}

int container_start(container_t* container) {
    if (!container) return -1;
    
    if (container->running) {
        DEBUG_WARN("Container already running");
        return -1;
    }
    
    container->running = true;
    DEBUG_INFO("Container started: %s", container->name);
    return 0;
}

int container_stop(container_t* container) {
    if (!container) return -1;
    
    if (!container->running) {
        DEBUG_WARN("Container not running");
        return -1;
    }
    
    container->running = false;
    DEBUG_INFO("Container stopped: %s", container->name);
    return 0;
}

int container_delete(container_t* container) {
    if (!container) return -1;
    
    if (container->running) {
        container_stop(container);
    }
    
    if (container->namespaces) {
        kfree(container->namespaces);
    }
    
    if (container->cgroup) {
        kfree(container->cgroup);
    }
    
    spinlock_lock(&container_lock);
    container_t** prev = &container_list_head;
    container_t* current = container_list_head;
    
    while (current) {
        if (current == container) {
            *prev = current->next;
            break;
        }
        prev = &current->next;
        current = current->next;
    }
    spinlock_unlock(&container_lock);
    
    kfree(container);
    DEBUG_INFO("Container deleted");
    return 0;
}

int container_list(container_t** containers, u32* count) {
    if (!containers || !count) return -1;
    
    spinlock_lock(&container_lock);
    
    u32 cnt = 0;
    container_t* c = container_list_head;
    while (c) {
        cnt++;
        c = c->next;
    }
    
    if (cnt > 0) {
        *containers = (container_t*)kmalloc(cnt * sizeof(container_t));
        if (*containers) {
            c = container_list_head;
            for (u32 i = 0; i < cnt && c; i++) {
                (*containers)[i] = *c;
                c = c->next;
            }
        }
    }
    
    *count = cnt;
    spinlock_unlock(&container_lock);
    
    return 0;
}

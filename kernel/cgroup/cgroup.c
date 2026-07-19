#include "cgroup.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "process.h"
#include "spinlock.h"

static cgroup_t* cgroup_root = NULL;
static spinlock_t cgroup_lock = SPINLOCK_INIT;
static u32 next_cg_id = 1;

void cgroup_init(void) {
    /* Create root cgroup */
    cgroup_root = (cgroup_t*)kzalloc(sizeof(cgroup_t));
    if (cgroup_root) {
        strncpy(cgroup_root->name, "/", sizeof(cgroup_root->name) - 1);
        cgroup_root->name[sizeof(cgroup_root->name) - 1] = '\0';
        cgroup_root->id = next_cg_id++;
    }
    
    DEBUG_INFO("Cgroup system initialized");
}

cgroup_t* cgroup_create(const char* name, cgroup_t* parent) {
    cgroup_t* cg = (cgroup_t*)kzalloc(sizeof(cgroup_t));
    if (!cg) {
        DEBUG_ERROR("Failed to allocate cgroup");
        return NULL;
    }
    
    strncpy(cg->name, name, sizeof(cg->name) - 1);
    cg->id = next_cg_id++;
    cg->parent = parent;
    cg->cpu_limit = 0; /* Unlimited */
    cg->memory_limit = 0; /* Unlimited */
    cg->pids_limit = 0; /* Unlimited */
    
    spinlock_lock(&cgroup_lock);
    if (parent) {
        cg->sibling = parent->children;
        parent->children = cg;
    } else {
        cg->sibling = cgroup_root;
        cgroup_root = cg;
    }
    spinlock_unlock(&cgroup_lock);
    
    DEBUG_INFO("Cgroup created: %s", name);
    return cg;
}

int cgroup_attach_process(cgroup_t* cg, u64 pid) {
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            spinlock_lock(&cgroup_lock);
            cg->process_count++;
            proc->cgroup_data = cg;
            spinlock_unlock(&cgroup_lock);
            DEBUG_INFO("Process %u attached to cgroup %s", (u32)pid, cg->name);
            return 0;
        }
        proc = proc->next;
    }
    
    return -1;
}

int cgroup_set_cpu_limit(cgroup_t* cg, u64 limit) {
    if (!cg) return -1;
    cg->cpu_limit = limit;
    return 0;
}

int cgroup_set_memory_limit(cgroup_t* cg, u64 limit) {
    if (!cg) return -1;
    cg->memory_limit = limit;
    return 0;
}

int cgroup_set_pids_limit(cgroup_t* cg, u32 limit) {
    if (!cg) return -1;
    cg->pids_limit = limit;
    return 0;
}

cgroup_t* cgroup_get_process(u64 pid) {
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            return (cgroup_t*)proc->cgroup_data;
        }
        proc = proc->next;
    }
    
    return NULL;
}

static void cgroup_destroy_children(cgroup_t* child) {
    while (child) {
        cgroup_t* next = child->sibling;
        if (child->children) {
            cgroup_destroy_children(child->children);
        }
        kfree(child);
        child = next;
    }
}

void cgroup_destroy(cgroup_t* cg) {
    if (!cg || cg == cgroup_root) {
        return;
    }

    spinlock_lock(&cgroup_lock);

    cgroup_t** link = &cgroup_root;
    if (cg->parent) {
        link = &cg->parent->children;
    }

    while (*link) {
        if (*link == cg) {
            *link = cg->sibling;
            break;
        }
        link = &(*link)->sibling;
    }

    spinlock_unlock(&cgroup_lock);

    cgroup_destroy_children(cg->children);
    kfree(cg);
}

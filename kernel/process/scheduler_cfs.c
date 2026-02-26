#include "scheduler.h"
#include "process.h"
#include "memory.h"
#include "stdio.h"
#include "debug.h"
#include "spinlock.h"
#include "interrupt.h"

/* CFS (Completely Fair Scheduler) implementation */

/* Red-black tree node for CFS */
typedef struct rb_node {
    u64 vruntime;
    process_t* proc;
    struct rb_node* left;
    struct rb_node* right;
    struct rb_node* parent;
    u8 color; /* 0 = black, 1 = red */
} rb_node_t;

static rb_node_t* cfs_root = NULL;
static process_t* idle_process = NULL;
static scheduler_stats_t stats = {0};
static spinlock_t scheduler_lock = SPINLOCK_INIT;

/* Red-black tree operations */
static void rb_rotate_left(rb_node_t** root, rb_node_t* node) {
    rb_node_t* right = node->right;
    node->right = right->left;
    if (right->left) {
        right->left->parent = node;
    }
    right->parent = node->parent;
    if (!node->parent) {
        *root = right;
    } else if (node == node->parent->left) {
        node->parent->left = right;
    } else {
        node->parent->right = right;
    }
    right->left = node;
    node->parent = right;
}

static void rb_rotate_right(rb_node_t** root, rb_node_t* node) {
    rb_node_t* left = node->left;
    node->left = left->right;
    if (left->right) {
        left->right->parent = node;
    }
    left->parent = node->parent;
    if (!node->parent) {
        *root = left;
    } else if (node == node->parent->right) {
        node->parent->right = left;
    } else {
        node->parent->left = left;
    }
    left->right = node;
    node->parent = left;
}

static void rb_insert(rb_node_t** root, rb_node_t* new_node) {
    rb_node_t* parent = NULL;
    rb_node_t* current = *root;
    
    while (current) {
        parent = current;
        if (new_node->vruntime < current->vruntime) {
            current = current->left;
        } else {
            current = current->right;
        }
    }
    
    new_node->parent = parent;
    new_node->color = 1; /* Red */
    new_node->left = NULL;
    new_node->right = NULL;
    
    if (!parent) {
        *root = new_node;
    } else if (new_node->vruntime < parent->vruntime) {
        parent->left = new_node;
    } else {
        parent->right = new_node;
    }
    
    /* Fix red-black tree properties */
    while (new_node != *root && new_node->parent->color == 1) {
        if (new_node->parent == new_node->parent->parent->left) {
            rb_node_t* uncle = new_node->parent->parent->right;
            if (uncle && uncle->color == 1) {
                new_node->parent->color = 0;
                uncle->color = 0;
                new_node->parent->parent->color = 1;
                new_node = new_node->parent->parent;
            } else {
                if (new_node == new_node->parent->right) {
                    new_node = new_node->parent;
                    rb_rotate_left(root, new_node);
                }
                new_node->parent->color = 0;
                new_node->parent->parent->color = 1;
                rb_rotate_right(root, new_node->parent->parent);
            }
        } else {
            rb_node_t* uncle = new_node->parent->parent->left;
            if (uncle && uncle->color == 1) {
                new_node->parent->color = 0;
                uncle->color = 0;
                new_node->parent->parent->color = 1;
                new_node = new_node->parent->parent;
            } else {
                if (new_node == new_node->parent->left) {
                    new_node = new_node->parent;
                    rb_rotate_right(root, new_node);
                }
                new_node->parent->color = 0;
                new_node->parent->parent->color = 1;
                rb_rotate_left(root, new_node->parent->parent);
            }
        }
    }
    (*root)->color = 0; /* Root is always black */
}

static rb_node_t* rb_find_min(rb_node_t* root) {
    while (root && root->left) {
        root = root->left;
    }
    return root;
}

static void rb_remove(rb_node_t** root, rb_node_t* node) {
    /* Simplified removal - would need full implementation */
    (void)root;
    (void)node;
}

u64 calc_vruntime(process_t* proc) {
    if (!proc) return 0;
    
    /* Virtual runtime = actual runtime / weight */
    /* Weight is based on nice value */
    int nice = proc->priority;
    u64 weight = 1024; /* Base weight */
    
    /* Adjust weight based on nice value */
    if (nice < 0) {
        weight = weight << (-nice);
    } else if (nice > 0) {
        weight = weight >> nice;
    }
    
    return proc->runtime * 1024 / weight;
}

void scheduler_init(void) {
    /* Create idle process */
    idle_process = (process_t*)kzalloc(sizeof(process_t));
    if (idle_process) {
        idle_process->pid = 0;
        idle_process->state = PROCESS_READY;
        idle_process->priority = PRIO_DEFAULT;
    }
    
    DEBUG_INFO("CFS scheduler initialized");
}

void schedule(void) {
    spinlock_lock_irq(&scheduler_lock);
    
    process_t* current = process_current();
    
    if (current && current->state == PROCESS_RUNNING) {
        /* Update virtual runtime */
        current->runtime++;
        u64 vruntime = calc_vruntime(current);
        
        /* Re-insert into tree */
        rb_node_t* node = (rb_node_t*)kmalloc(sizeof(rb_node_t));
        if (node) {
            node->vruntime = vruntime;
            node->proc = current;
            rb_insert(&cfs_root, node);
        }
        
        current->state = PROCESS_READY;
    }
    
    /* Find next process (leftmost = smallest vruntime) */
    rb_node_t* next_node = rb_find_min(cfs_root);
    process_t* next = NULL;
    
    if (next_node) {
        next = next_node->proc;
        rb_remove(&cfs_root, next_node);
        kfree(next_node);
    }
    
    if (!next) {
        next = idle_process;
    }
    
    if (next && next != current) {
        next->state = PROCESS_RUNNING;
        stats.total_switches++;
        
        if (current) {
            context_switch(current, next);
        } else {
            /* First switch */
            asm volatile("mov %0, %%rsp" : : "r"(next->rsp));
            asm volatile("jmp *%0" : : "r"(next->rip));
        }
    }
    
    spinlock_unlock_irq(&scheduler_lock);
}

int setpriority(u64 pid, int priority) {
    if (priority < PRIO_MIN || priority > PRIO_MAX) {
        return -1;
    }
    
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            proc->priority = priority;
            return 0;
        }
        proc = proc->next;
    }
    
    return -1;
}

int getpriority(u64 pid) {
    extern process_t* process_list;
    process_t* proc = process_list;
    
    while (proc) {
        if (proc->pid == pid) {
            return proc->priority;
        }
        proc = proc->next;
    }
    
    return -1;
}

int sched_setscheduler(u64 pid, int policy, int priority) {
    (void)policy; /* For now, only CFS */
    return setpriority(pid, priority);
}

void scheduler_get_stats(scheduler_stats_t* out_stats) {
    if (out_stats) {
        *out_stats = stats;
    }
}

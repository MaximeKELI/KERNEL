#include "scheduler.h"
#include "ai_types.h"
#include "ai_learn.h"
#include "sched_stats.h"
#include "process.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "debug.h"
#include "spinlock.h"
#include "interrupt.h"

/*
 * CFS (Completely Fair Scheduler) core with real preemptive multitasking.
 *
 * The runqueue is a red-black tree keyed by vruntime; the tree node is embedded
 * in every process_t (process_t.sched_node) so scheduling never allocates. The
 * leftmost node (smallest vruntime) is the next task to run. Preemption is
 * driven by the PIT timer (scheduler_timer_preempt) and the actual register
 * save/restore is done by switch_to() (kernel/asm/context_switch.S).
 */

#define SCHED_QUANTUM_TICKS 3   /* PIT ticks (10ms each) a task runs before preemption */

extern process_t* current_process;   /* defined in scheduler.c */
extern process_t* process_list;

static sched_node_t* cfs_root = NULL;
static process_t* idle_process = NULL;
static process_t boot_task;           /* represents the initial kernel context */
static scheduler_stats_t stats = {0};
static u64 min_vruntime = 0;          /* fair placement baseline for woken tasks */
static volatile int preempt_count = 0;

/* ------------------------------------------------------------------ */
/* Red-black tree (keyed by sched_node.vruntime)                       */
/* ------------------------------------------------------------------ */

static inline int node_is_red(sched_node_t* n) { return n && n->color == 1; }

static void rb_rotate_left(sched_node_t** root, sched_node_t* node) {
    sched_node_t* right = node->right;
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

static void rb_rotate_right(sched_node_t** root, sched_node_t* node) {
    sched_node_t* left = node->left;
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

static void rb_insert(sched_node_t** root, sched_node_t* new_node) {
    sched_node_t* parent = NULL;
    sched_node_t* current = *root;

    while (current) {
        parent = current;
        if (new_node->vruntime < current->vruntime) {
            current = current->left;
        } else {
            current = current->right;
        }
    }

    new_node->parent = parent;
    new_node->color = 1; /* red */
    new_node->left = NULL;
    new_node->right = NULL;

    if (!parent) {
        *root = new_node;
    } else if (new_node->vruntime < parent->vruntime) {
        parent->left = new_node;
    } else {
        parent->right = new_node;
    }

    /* Restore red-black invariants after insertion. */
    while (new_node != *root && new_node->parent->color == 1) {
        if (new_node->parent == new_node->parent->parent->left) {
            sched_node_t* uncle = new_node->parent->parent->right;
            if (node_is_red(uncle)) {
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
            sched_node_t* uncle = new_node->parent->parent->left;
            if (node_is_red(uncle)) {
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
    (*root)->color = 0; /* root is always black */
}

static sched_node_t* rb_find_min(sched_node_t* root) {
    while (root && root->left) {
        root = root->left;
    }
    return root;
}

static sched_node_t* rb_subtree_min(sched_node_t* n) {
    while (n->left) {
        n = n->left;
    }
    return n;
}

/* Replace subtree rooted at u with subtree rooted at v (v may be NULL). */
static void rb_transplant(sched_node_t** root, sched_node_t* u, sched_node_t* v) {
    if (!u->parent) {
        *root = v;
    } else if (u == u->parent->left) {
        u->parent->left = v;
    } else {
        u->parent->right = v;
    }
    if (v) {
        v->parent = u->parent;
    }
}

/*
 * Restore red-black invariants after removing a black node. `x` is the node that
 * moved into the removed node's place (possibly NULL), `parent` its parent (the
 * explicit parent lets us cope with NULL x, since we have no sentinel).
 */
static void rb_delete_fixup(sched_node_t** root, sched_node_t* x, sched_node_t* parent) {
    while (x != *root && !node_is_red(x)) {
        if (x == parent->left) {
            sched_node_t* w = parent->right;
            if (node_is_red(w)) {
                w->color = 0;
                parent->color = 1;
                rb_rotate_left(root, parent);
                w = parent->right;
            }
            if (!node_is_red(w->left) && !node_is_red(w->right)) {
                w->color = 1;
                x = parent;
                parent = x->parent;
            } else {
                if (!node_is_red(w->right)) {
                    if (w->left) w->left->color = 0;
                    w->color = 1;
                    rb_rotate_right(root, w);
                    w = parent->right;
                }
                w->color = parent->color;
                parent->color = 0;
                if (w->right) w->right->color = 0;
                rb_rotate_left(root, parent);
                x = *root;
                break;
            }
        } else {
            sched_node_t* w = parent->left;
            if (node_is_red(w)) {
                w->color = 0;
                parent->color = 1;
                rb_rotate_right(root, parent);
                w = parent->left;
            }
            if (!node_is_red(w->right) && !node_is_red(w->left)) {
                w->color = 1;
                x = parent;
                parent = x->parent;
            } else {
                if (!node_is_red(w->left)) {
                    if (w->right) w->right->color = 0;
                    w->color = 1;
                    rb_rotate_left(root, w);
                    w = parent->left;
                }
                w->color = parent->color;
                parent->color = 0;
                if (w->left) w->left->color = 0;
                rb_rotate_right(root, parent);
                x = *root;
                break;
            }
        }
    }
    if (x) {
        x->color = 0;
    }
}

static void rb_erase(sched_node_t** root, sched_node_t* z) {
    sched_node_t* y = z;
    sched_node_t* x;
    sched_node_t* x_parent;
    int y_original_color = y->color;

    if (!z->left) {
        x = z->right;
        x_parent = z->parent;
        rb_transplant(root, z, z->right);
    } else if (!z->right) {
        x = z->left;
        x_parent = z->parent;
        rb_transplant(root, z, z->left);
    } else {
        y = rb_subtree_min(z->right);
        y_original_color = y->color;
        x = y->right;
        if (y->parent == z) {
            x_parent = y;
        } else {
            x_parent = y->parent;
            rb_transplant(root, y, y->right);
            y->right = z->right;
            y->right->parent = y;
        }
        rb_transplant(root, z, y);
        y->left = z->left;
        y->left->parent = y;
        y->color = z->color;
    }

    if (y_original_color == 0) {  /* a black node was removed */
        rb_delete_fixup(root, x, x_parent);
    }

    z->left = z->right = z->parent = NULL;
}

/* ------------------------------------------------------------------ */
/* vruntime accounting                                                 */
/* ------------------------------------------------------------------ */

u64 calc_vruntime(process_t* proc) {
    if (!proc) return 0;

    int nice = (int)proc->priority;
    u64 weight = 1024; /* base weight */

    if (nice < 0) {
        weight = weight << (-nice);
    } else if (nice > 0) {
        weight = weight >> nice;
    }

    /* AI vruntime bias: favor I/O and network waiters. */
    switch (proc->ai_class) {
    case AI_CLASS_IO:
    case AI_CLASS_NET:
        weight += weight / 4 + (ai_learn_weight(AI_ACT_PRIO_BOOST) / 64);
        break;
    case AI_CLASS_CPU:
        if (weight > 256) {
            weight -= weight / 8;
        }
        break;
    default:
        break;
    }

    if (weight < 64) {
        weight = 64;
    }

    return proc->runtime * 1024 / weight;
}

static void rq_insert(process_t* p, u64 vruntime) {
    p->sched_node.proc = p;
    p->sched_node.vruntime = vruntime;
    rb_insert(&cfs_root, &p->sched_node);
    p->sched_node.on_rq = 1;
}

/* ------------------------------------------------------------------ */
/* Init / preemption control                                           */
/* ------------------------------------------------------------------ */

void scheduler_init(void) {
    static bool inited = false;
    if (inited) {
        return;  /* called from both minimal and extended init paths */
    }
    inited = true;

    /*
     * The context executing right now becomes the initial (boot/idle-capable)
     * task. Its saved rsp is written by the first switch_to() away from it.
     */
    memset(&boot_task, 0, sizeof(boot_task));
    boot_task.pid = 0;
    boot_task.state = PROCESS_RUNNING;
    boot_task.priority = PRIO_DEFAULT;
    boot_task.time_slice_left = SCHED_QUANTUM_TICKS;
    boot_task.cr3 = vmm_get_cr3();
    boot_task.sched_node.proc = &boot_task;
    boot_task.refcount.count = 1;
    spinlock_init(&boot_task.lock);
    current_process = &boot_task;

    /* Dedicated idle task, run only when the runqueue is otherwise empty. */
    idle_process = kthread_create_stopped((void (*)(void*))idle_task, NULL, 16 * 1024);

    DEBUG_INFO("%s", "CFS scheduler initialized (preemptive)");
}

void preempt_disable(void) {
    __atomic_add_fetch(&preempt_count, 1, __ATOMIC_SEQ_CST);
}

void preempt_enable(void) {
    __atomic_sub_fetch(&preempt_count, 1, __ATOMIC_SEQ_CST);
}

void scheduler_tick(void) {
    /*
     * One PIT tick of CPU time has elapsed. Attribute it to total runtime and,
     * when nothing but the idle task is running, to idle time. These counters
     * feed the AI monitor's CPU-usage computation.
     */
    stats.total_runtime++;
    process_t* current = current_process;
    if (!current || current == idle_process || current->pid == 0) {
        stats.idle_time++;
    }
}

void scheduler_timer_preempt(void) {
    process_t* cur = current_process;
    if (!cur) {
        return;
    }
    if (preempt_count != 0) {
        return;  /* inside a non-preemptible critical section */
    }
    if (cur->time_slice_left > 0) {
        cur->time_slice_left--;
    }
    if (cur->time_slice_left == 0) {
        schedule();  /* time slice expired: pick another runnable task */
    }
}

/* ------------------------------------------------------------------ */
/* wake / schedule / switch                                            */
/* ------------------------------------------------------------------ */

void wake_up_process(process_t* p) {
    if (!p) {
        return;
    }
    u64 flags = local_irq_save();
    if (!p->sched_node.on_rq && p != current_process) {
        p->state = PROCESS_READY;
        rq_insert(p, min_vruntime);
    }
    local_irq_restore(flags);
}

/*
 * finish_task_switch: the first thing every freshly-scheduled *new* task runs
 * (via kthread_trampoline). On a single CPU there is no runqueue lock to drop,
 * so we simply re-enable interrupts to make the new task preemptible.
 */
void schedule_tail(void) {
    enable_interrupts();
}

void schedule(void) {
    sched_stats_record_schedule();

    u64 flags = local_irq_save();

    process_t* prev = current_process;

    /* Requeue the outgoing task if it is still runnable. */
    if (prev && prev->state == PROCESS_RUNNING) {
        prev->runtime++;
        rq_insert(prev, calc_vruntime(prev));
        prev->state = PROCESS_READY;
    }

    /* Pick the leftmost task (smallest vruntime), or idle if the tree is empty. */
    process_t* next;
    sched_node_t* nn = rb_find_min(cfs_root);
    if (nn) {
        rb_erase(&cfs_root, nn);
        nn->on_rq = 0;
        min_vruntime = nn->vruntime;
        next = nn->proc;
    } else {
        next = idle_process ? idle_process : prev;
    }

    if (!next) {
        local_irq_restore(flags);
        return;
    }

    next->state = PROCESS_RUNNING;
    next->time_slice_left = SCHED_QUANTUM_TICKS;

    if (next != prev) {
        stats.total_switches++;
        sched_stats_record_switch();
        current_process = next;
        switch_to(&prev->rsp, &next->rsp);
        /* Control returns here only when `prev` is scheduled again later. */
    }

    local_irq_restore(flags);
}

/* ------------------------------------------------------------------ */
/* priorities / stats                                                  */
/* ------------------------------------------------------------------ */

int setpriority(u64 pid, int priority) {
    if (priority < PRIO_MIN || priority > PRIO_MAX) {
        return -1;
    }

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
    process_t* proc = process_list;
    while (proc) {
        if (proc->pid == pid) {
            return (int)proc->priority;
        }
        proc = proc->next;
    }
    return -1;
}

void scheduler_get_stats(scheduler_stats_t* out_stats) {
    if (out_stats) {
        *out_stats = stats;
    }
}

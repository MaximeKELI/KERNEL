#include "landlock.h"
#include "process.h"
#include "memory.h"
#include "string.h"
#include "stdio.h"
#include "spinlock.h"
#include "debug.h"
#include "types.h"

#define LANDLOCK_MAX_RULES 64
#define LANDLOCK_PATH_LEN 128

typedef struct landlock_rule {
    u64 pid;
    char path[LANDLOCK_PATH_LEN];
    u32 access_mask;
    bool deny;
    bool active;
} landlock_rule_t;

static landlock_rule_t rules[LANDLOCK_MAX_RULES];
static u32 rule_count = 0;
static bool landlock_enabled = false;
static spinlock_t landlock_lock = SPINLOCK_INIT;

void landlock_init(void) {
    memset(rules, 0, sizeof(rules));
    rule_count = 0;
    landlock_enabled = true;
    printk("[landlock] LSM ready\n");
}

int landlock_add_rule(u64 pid, const char* path, u32 access_mask, bool deny) {
    if (!landlock_enabled || !path) {
        return -1;
    }

    spinlock_lock(&landlock_lock);

    landlock_rule_t* slot = NULL;
    for (u32 i = 0; i < LANDLOCK_MAX_RULES; i++) {
        if (!rules[i].active) {
            slot = &rules[i];
            break;
        }
    }

    if (!slot) {
        spinlock_unlock(&landlock_lock);
        return -1;
    }

    slot->pid = pid;
    strncpy(slot->path, path, LANDLOCK_PATH_LEN - 1);
    slot->path[LANDLOCK_PATH_LEN - 1] = '\0';
    slot->access_mask = access_mask;
    slot->deny = deny;
    slot->active = true;
    rule_count++;

    spinlock_unlock(&landlock_lock);
    return 0;
}

bool landlock_check_access(u64 pid, const char* path, u32 access) {
    if (!landlock_enabled || !path) {
        return true;
    }

    bool allowed = true;

    spinlock_lock(&landlock_lock);
    for (u32 i = 0; i < LANDLOCK_MAX_RULES; i++) {
        if (!rules[i].active) {
            continue;
        }
        if (rules[i].pid != 0 && rules[i].pid != pid) {
            continue;
        }
        if (strncmp(rules[i].path, path, LANDLOCK_PATH_LEN) != 0) {
            continue;
        }
        if (rules[i].access_mask & access) {
            allowed = !rules[i].deny;
            break;
        }
    }
    spinlock_unlock(&landlock_lock);

    return allowed;
}

void landlock_clear_rules(u64 pid) {
    spinlock_lock(&landlock_lock);
    for (u32 i = 0; i < LANDLOCK_MAX_RULES; i++) {
        if (rules[i].active && (pid == 0 || rules[i].pid == pid)) {
            rules[i].active = false;
            if (rule_count > 0) {
                rule_count--;
            }
        }
    }
    spinlock_unlock(&landlock_lock);
}

u32 landlock_rule_count(void) {
    return rule_count;
}

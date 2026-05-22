#include "ext2.h"
#include "memory.h"
#include "stdio.h"
#include "string.h"
#include "spinlock.h"

#define JOURNAL_SLOTS 64

typedef struct ext2_journal_rec {
    u32 ino;
    u32 block;
    u32 op;
    u64 seq;
    bool valid;
} ext2_journal_rec_t;

static ext2_journal_rec_t journal[JOURNAL_SLOTS];
static u64 journal_seq = 1;
static spinlock_t journal_lock = SPINLOCK_INIT;

void ext2_journal_init(void) {
    memset(journal, 0, sizeof(journal));
    printk("[ext2] journal: %u slots (metadata logging)\n", JOURNAL_SLOTS);
}

void ext2_journal_log(u32 ino, u32 block, u32 op) {
    spinlock_lock(&journal_lock);
    for (u32 i = 0; i < JOURNAL_SLOTS; i++) {
        if (!journal[i].valid) {
            journal[i].ino = ino;
            journal[i].block = block;
            journal[i].op = op;
            journal[i].seq = journal_seq++;
            journal[i].valid = true;
            spinlock_unlock(&journal_lock);
            return;
        }
    }
    spinlock_unlock(&journal_lock);
}

void ext2_journal_commit(void) {
    spinlock_lock(&journal_lock);
    for (u32 i = 0; i < JOURNAL_SLOTS; i++) {
        journal[i].valid = false;
    }
    spinlock_unlock(&journal_lock);
}

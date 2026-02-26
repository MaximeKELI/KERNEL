#ifndef ROBUST_FUTEX_H
#define ROBUST_FUTEX_H

#include "types.h"
#include "futex.h"

/* Robust futex list entry */
typedef struct robust_list {
    struct robust_list* next;
} robust_list_t;

/* Robust futex head */
typedef struct robust_list_head {
    robust_list_t* list;
    long futex_offset;
    robust_list_t* list_op_pending;
} robust_list_head_t;

/* Set robust list */
int set_robust_list(const robust_list_head_t* head, size_t len);

/* Get robust list */
int get_robust_list(robust_list_head_t* head, size_t* len);

/* Handle robust futex on process exit */
void robust_futex_exit(void);

#endif /* ROBUST_FUTEX_H */

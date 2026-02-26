#ifndef LIVEPATCH_H
#define LIVEPATCH_H

#include "types.h"

/* Live patch structure */
typedef struct livepatch {
    char name[64];
    void* old_func;
    void* new_func;
    bool enabled;
    struct livepatch* next;
} livepatch_t;

/* Initialize live patching */
void livepatch_init(void);

/* Register live patch */
int livepatch_register(livepatch_t* patch);

/* Enable live patch */
int livepatch_enable(const char* name);

/* Disable live patch */
int livepatch_disable(const char* name);

/* Apply live patch */
int livepatch_apply(livepatch_t* patch);

#endif /* LIVEPATCH_H */

#ifndef ROP_PROTECTION_H
#define ROP_PROTECTION_H

#include "types.h"

/* ROP protection types */
#define ROP_PROT_NONE    0
#define ROP_PROT_CET     1  /* Control-flow Enforcement Technology */
#define ROP_PROT_SHADOW  2  /* Shadow stack */

/* Initialize ROP protection */
void rop_protection_init(void);

/* Enable CET */
int rop_protection_enable_cet(void);

/* Enable shadow stack */
int rop_protection_enable_shadow_stack(void);

/* Verify return address */
bool rop_protection_verify_return(void* return_addr);

/* Check if protection enabled */
bool rop_protection_is_enabled(void);

#endif /* ROP_PROTECTION_H */

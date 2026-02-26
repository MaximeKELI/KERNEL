#ifndef SECURITY_HARDENING_H
#define SECURITY_HARDENING_H

#include "types.h"

/* Security features */
#define SECURITY_SMEP_ENABLED    (1 << 0)
#define SECURITY_SMAP_ENABLED    (1 << 1)
#define SECURITY_KPTI_ENABLED    (1 << 2)
#define SECURITY_RETPOLINE       (1 << 3)
#define SECURITY_CFI_ENABLED     (1 << 4)
#define SECURITY_STACK_PROT      (1 << 5)
#define SECURITY_LOCKDOWN        (1 << 6)
#define SECURITY_MEM_ENCRYPT     (1 << 7)

/* Initialize security hardening */
void security_hardening_init(void);

/* Enable SMEP (Supervisor Mode Execution Prevention) */
void security_enable_smep(void);

/* Enable SMAP (Supervisor Mode Access Prevention) */
void security_enable_smap(void);

/* Enable KPTI (Kernel Page Table Isolation) */
void security_enable_kpti(void);

/* Enable Retpoline */
void security_enable_retpoline(void);

/* Enable CFI */
void security_enable_cfi(void);

/* Enable kernel lockdown */
void security_enable_lockdown(void);

/* Check if feature enabled */
bool security_is_enabled(u32 feature);

#endif /* SECURITY_HARDENING_H */

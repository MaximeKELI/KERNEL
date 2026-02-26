#ifndef KEXEC_H
#define KEXEC_H

#include "types.h"

/* Initialize kexec */
void kexec_init(void);

/* Load new kernel */
int kexec_load(void* kernel_image, size_t size, void* cmdline);

/* Execute new kernel */
int kexec_exec(void);

/* Prepare for kexec */
int kexec_prepare(void);

#endif /* KEXEC_H */

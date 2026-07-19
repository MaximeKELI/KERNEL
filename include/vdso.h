#ifndef VDSO_H
#define VDSO_H

#include "types.h"

void vdso_init(void);
void vdso_update(void);
u64 vdso_user_base(void);
u64 vdso_ehdr_addr(void);
int vdso_selftest(void);
int vdso_map_user(void);

#endif /* VDSO_H */

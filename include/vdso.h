#ifndef VDSO_H
#define VDSO_H

#include "types.h"

void vdso_init(void);
void vdso_update(void);
u64 vdso_user_base(void);

#endif /* VDSO_H */

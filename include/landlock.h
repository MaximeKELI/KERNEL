#ifndef LANDLOCK_H
#define LANDLOCK_H

#include "types.h"

#define LANDLOCK_ACCESS_FS_READ  0x1
#define LANDLOCK_ACCESS_FS_WRITE 0x2
#define LANDLOCK_ACCESS_FS_EXEC  0x4

void landlock_init(void);
int landlock_add_rule(u64 pid, const char* path, u32 access_mask, bool deny);
bool landlock_check_access(u64 pid, const char* path, u32 access);
void landlock_clear_rules(u64 pid);
u32 landlock_rule_count(void);

#endif /* LANDLOCK_H */

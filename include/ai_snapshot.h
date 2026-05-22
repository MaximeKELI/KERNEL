#ifndef AI_SNAPSHOT_H
#define AI_SNAPSHOT_H

#include "types.h"

void ai_snapshot_init(void);
void ai_snapshot_capture(void);
void ai_snapshot_compare(void);
bool ai_snapshot_has_baseline(void);

#endif /* AI_SNAPSHOT_H */

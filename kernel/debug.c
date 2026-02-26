#include "debug.h"

u32 debug_level = DEBUG_LEVEL_INFO;

void debug_set_level(u32 level) {
    debug_level = level;
}

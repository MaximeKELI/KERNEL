#ifndef PC_SPEAKER_H
#define PC_SPEAKER_H

#include "types.h"

void pc_speaker_init(void);
void pc_speaker_beep(u32 freq_hz, u32 ms);
void pc_speaker_off(void);

#endif /* PC_SPEAKER_H */

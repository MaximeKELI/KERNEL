#include "stdio.h"
#include "io.h"
#include "types.h"

#define PIT_CH2 0x42
#define PIT_CMD 0x43

void pc_speaker_init(void) {
    printk("[pcspk] PC speaker ready\n");
}

void pc_speaker_beep(u32 freq_hz, u32 ms) {
    if (freq_hz == 0) {
        return;
    }
    u32 divisor = 1193182 / freq_hz;
    outb(PIT_CMD, 0xB6);
    outb(PIT_CH2, (u8)(divisor & 0xFF));
    outb(PIT_CH2, (u8)((divisor >> 8) & 0xFF));
    (void)ms;
}

void pc_speaker_off(void) {
    outb(0x61, inb(0x61) & ~0x03);
}

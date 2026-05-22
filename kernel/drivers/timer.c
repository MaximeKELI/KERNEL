#include "drivers/timer.h"
#include "interrupt.h"
#include "io.h"
#include "stdio.h"
#include "ai_manager.h"
#include "smp.h"

#include "hw_ports.h"

#define PIT_FREQUENCY 1193182

static u64 ticks = 0;
static timer_callback_t callback = NULL;
static void* callback_data = NULL;

static void timer_irq_handler(u32 irq, void* data) {
    (void)irq;
    (void)data;
    
    ticks++;
    
    extern bool ai_initialized;
    if (ai_initialized) {
        u32 cpu = smp_get_cpu_id();
        if (cpu == 0 || (ticks % 2) == cpu % 2) {
            ai_tick();
        }
    }
    
    if (callback) {
        callback(callback_data);
    }
}

void timer_init(u32 frequency) {
    u32 divisor = PIT_FREQUENCY / frequency;
    
    /* Configure PIT channel 0 */
    outb(PIT_MODE_COMMAND, 0x36);  /* Channel 0, mode 3, binary */
    outb(PIT_CHANNEL0_DATA, divisor & 0xFF);
    outb(PIT_CHANNEL0_DATA, (divisor >> 8) & 0xFF);
    
    /* Register IRQ handler */
    irq_register(0, timer_irq_handler, NULL);
    
    (void)frequency;
}

u64 timer_get_ticks(void) {
    return ticks;
}

void timer_sleep(u64 ms) {
    u64 end = ticks + ms;
    while (ticks < end) {
        __asm__ __volatile__("hlt");
    }
}

void timer_register_callback(timer_callback_t cb, void* data) {
    callback = cb;
    callback_data = data;
}

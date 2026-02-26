#include "drivers/timer.h"
#include "interrupt.h"
#include "io.h"
#include "stdio.h"
#include "ai_manager.h"

#define PIT_CHANNEL0_DATA 0x40
#define PIT_CHANNEL1_DATA 0x41
#define PIT_CHANNEL2_DATA 0x42
#define PIT_MODE_COMMAND 0x43

#define PIT_FREQUENCY 1193182

static u64 ticks = 0;
static timer_callback_t callback = NULL;
static void* callback_data = NULL;

static void timer_irq_handler(u32 irq, void* data) {
    (void)irq;
    (void)data;
    
    ticks++;
    
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
    
    printk("Timer: Initialized at %u Hz\n", frequency);
}

u64 timer_get_ticks(void) {
    return ticks;
}

void timer_sleep(u64 ms) {
    u64 end = ticks + ms;
    while (ticks < end) {
        asm volatile("hlt");
    }
}

void timer_register_callback(timer_callback_t cb, void* data) {
    callback = cb;
    callback_data = data;
}

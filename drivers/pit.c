/*
 * PrabroroOS — drivers/pit.c
 * PIT 8253/8254 — Channel 0, IRQ0, system timer
 */
#include "pit.h"
#include "../include/common.h"
#include "../cpu/pic.h"
#include "../kernel/types.h"

extern void register_irq_handler(uint8_t irq, void (*handler)(void *));

#define PIT_CMD   0x43
#define PIT_CH0   0x40
#define PIT_BASE  1193182U

#define MAX_CALLBACKS 8

static volatile uint32_t  pit_ticks = 0;
static tick_callback_t    callbacks[MAX_CALLBACKS];
static int                cb_count = 0;

void pit_irq_handler(void *regs) {
    (void)regs;
    pit_ticks++;
    for (int i = 0; i < cb_count; i++)
        if (callbacks[i]) callbacks[i](pit_ticks);
    pic_send_eoi(0);
}

void pit_init(uint32_t frequency_hz) {
    if (frequency_hz == 0) frequency_hz = 100;
    uint32_t divisor = PIT_BASE / frequency_hz;
    outb(PIT_CMD, 0x34);                        /* Channel 0, lobyte/hibyte, Mode 2 */
    outb(PIT_CH0, (uint8_t)(divisor & 0xFF));
    outb(PIT_CH0, (uint8_t)((divisor >> 8) & 0xFF));
    cb_count  = 0;
    pit_ticks = 0;
    register_irq_handler(0, pit_irq_handler);
}

uint32_t pit_get_ticks(void) {
    return pit_ticks;
}

void pit_register_callback(tick_callback_t cb) {
    if (cb && cb_count < MAX_CALLBACKS)
        callbacks[cb_count++] = cb;
}

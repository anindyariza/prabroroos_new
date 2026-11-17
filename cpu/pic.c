#include "pic.h"
#include "../include/common.h"

/* Primary (Master) PIC I/O ports */
#define PIC1_CMD        0x20
#define PIC1_DATA       0x21

/* Secondary (Slave) PIC I/O ports */
#define PIC2_CMD        0xA0
#define PIC2_DATA       0xA1

/* End of Interrupt command code */
#define PIC_EOI         0x20

/* Initialization Control Word 1 (ICW1) flags */
#define ICW1_ICW4       0x01    /* Expect ICW4 during initialization */
#define ICW1_SINGLE     0x02    /* Single (cascade) mode */
#define ICW1_INTERVAL4  0x04    /* Call address interval 4 (8) */
#define ICW1_LEVEL      0x08    /* Level triggered (edge) mode */
#define ICW1_INIT       0x10    /* Initialization bit */

/* Initialization Control Word 4 (ICW4) flags */
#define ICW4_8086       0x01    /* 8086/88 (MCS-80/85) mode */
#define ICW4_AUTO       0x02    /* Auto (normal) EOI */
#define ICW4_BUF_SLAVE  0x08    /* Buffered mode/slave */
#define ICW4_BUF_MASTER 0x0C    /* Buffered mode/master */
#define ICW4_SFNM       0x10    /* Special fully nested (not) */

void pic_init(void) {
    /* ICW1: Start initialization sequence on both PICs */
    outb(PIC1_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();
    outb(PIC2_CMD, ICW1_INIT | ICW1_ICW4);
    io_wait();

    /* ICW2: Set vector offsets */
    /* Master PIC remap to vectors 0x20 - 0x27 (32 - 39) */
    outb(PIC1_DATA, 0x20);
    io_wait();
    /* Slave PIC remap to vectors 0x28 - 0x2F (40 - 47) */
    outb(PIC2_DATA, 0x28);
    io_wait();

    /* ICW3: Configure cascading */
    /* Master PIC: Tell it that there is a slave PIC at IRQ2 (pin 2, bit 2: 0b00000100 = 0x04) */
    outb(PIC1_DATA, 0x04);
    io_wait();
    /* Slave PIC: Tell it its cascade identity (IRQ2: 0b00000010 = 0x02) */
    outb(PIC2_DATA, 0x02);
    io_wait();

    /* ICW4: Set operational environment (8086/88 mode) */
    outb(PIC1_DATA, ICW4_8086);
    io_wait();
    outb(PIC2_DATA, ICW4_8086);
    io_wait();

    /*
     * Configure Interrupt Mask Registers (IMR):
     * Unmask IRQ0 (timer, bit 0 = 0) and IRQ1 (keyboard, bit 1 = 0).
     * Mask all other lines on Master PIC (bits 2..7 = 1) -> 0xFC (11111100b).
     * Mask all lines on Slave PIC -> 0xFF (11111111b).
     */
    outb(PIC1_DATA, 0xFC);
    io_wait();
    outb(PIC2_DATA, 0xFF);
    io_wait();
}

void pic_send_eoi(uint8_t irq) {
    /* If the IRQ came from the Slave PIC (IRQ 8-15), issue EOI to Slave first */
    if (irq >= 8) {
        outb(PIC2_CMD, PIC_EOI);
    }
    /* Always issue EOI to Master PIC */
    outb(PIC1_CMD, PIC_EOI);
}

void pic_mask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = (uint8_t)(inb(port) | (1 << irq));
    outb(port, value);
}

void pic_unmask(uint8_t irq) {
    uint16_t port;
    uint8_t value;

    if (irq < 8) {
        port = PIC1_DATA;
    } else {
        port = PIC2_DATA;
        irq -= 8;
    }

    value = (uint8_t)(inb(port) & ~(1 << irq));
    outb(port, value);
}

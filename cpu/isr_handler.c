#include "isr.h"
#include "pic.h"
#include "../include/common.h"

#define IRQ_COUNT 16

/* Registered IRQ handler dispatch table */
static irq_handler_t irq_handlers[IRQ_COUNT] = { NULL };

/* Standard x86 Exception descriptions */
static const char *exception_messages[32] = {
    "Divide-by-zero Error",
    "Debug",
    "Non-maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Reserved",
    "Hypervisor Injection Exception",
    "VMM Communication Exception",
    "Security Exception",
    "Reserved"
};

/*
 * Direct VGA text buffer writer for kernel panic output (0xB8000)
 */
static volatile uint16_t * const vga_buffer = (volatile uint16_t *)0xB8000;

static void panic_print_char(char c, uint8_t color, int *row, int *col) {
    if (c == '\n') {
        *col = 0;
        (*row)++;
        return;
    }
    vga_buffer[(*row) * 80 + (*col)] = (uint16_t)((color << 8) | (uint8_t)c);
    (*col)++;
    if (*col >= 80) {
        *col = 0;
        (*row)++;
    }
}

static void panic_print_str(const char *str, uint8_t color, int *row, int *col) {
    while (*str) {
        panic_print_char(*str++, color, row, col);
    }
}

static void panic_print_hex(uint32_t val, uint8_t color, int *row, int *col) {
    const char hex_chars[] = "0123456789ABCDEF";
    panic_print_str("0x", color, row, col);
    for (int i = 28; i >= 0; i -= 4) {
        panic_print_char(hex_chars[(val >> i) & 0x0F], color, row, col);
    }
}

static void panic_print_dec(uint32_t val, uint8_t color, int *row, int *col) {
    char buf[12];
    int idx = 0;
    if (val == 0) {
        panic_print_char('0', color, row, col);
        return;
    }
    while (val > 0) {
        buf[idx++] = (char)('0' + (val % 10));
        val /= 10;
    }
    for (int i = idx - 1; i >= 0; i--) {
        panic_print_char(buf[i], color, row, col);
    }
}

/*
 * Direct serial output to COM1 port (0x3F8) for headless/log capture
 */
static void serial_write_char(char c) {
    outb(0x3F8, (uint8_t)c);
}

static void serial_write_str(const char *s) {
    while (*s) {
        if (*s == '\n') {
            serial_write_char('\r');
        }
        serial_write_char(*s++);
    }
}

static void serial_write_hex(uint32_t val) {
    const char hex_chars[] = "0123456789ABCDEF";
    serial_write_str("0x");
    for (int i = 28; i >= 0; i -= 4) {
        serial_write_char(hex_chars[(val >> i) & 0x0F]);
    }
}

static void serial_write_dec(uint32_t val) {
    char buf[12];
    int idx = 0;
    if (val == 0) {
        serial_write_char('0');
        return;
    }
    while (val > 0) {
        buf[idx++] = (char)('0' + (val % 10));
        val /= 10;
    }
    for (int i = idx - 1; i >= 0; i--) {
        serial_write_char(buf[i]);
    }
}

/*
 * CPU Exception Handler - prints exception information and halts the CPU
 */
void isr_handler(registers_t *regs) {
    const char *desc = (regs->int_no < 32) ? exception_messages[regs->int_no] : "Unknown Exception";

    /* Display panic screen on VGA (White on Red: 0x4F, Yellow on Red: 0x4E) */
    int row = 0, col = 0;
    for (int r = 0; r < 25; r++) {
        for (int c = 0; c < 80; c++) {
            vga_buffer[r * 80 + c] = (uint16_t)((0x4F << 8) | ' ');
        }
    }

    panic_print_str("========================================", 0x4F, &row, &col);
    panic_print_str(" KERNEL PANIC ", 0x4E, &row, &col);
    panic_print_str("========================================\n\n", 0x4F, &row, &col);

    panic_print_str("Exception:   ", 0x4F, &row, &col);
    panic_print_str(desc, 0x4E, &row, &col);
    panic_print_str(" (Vector ", 0x4F, &row, &col);
    panic_print_dec(regs->int_no, 0x4E, &row, &col);
    panic_print_str(", Error Code: ", 0x4F, &row, &col);
    panic_print_hex(regs->err_code, 0x4E, &row, &col);
    panic_print_str(")\n\n", 0x4F, &row, &col);

    panic_print_str("EIP: ", 0x4F, &row, &col);
    panic_print_hex(regs->eip, 0x4E, &row, &col);
    panic_print_str("  CS:     ", 0x4F, &row, &col);
    panic_print_hex(regs->cs, 0x4E, &row, &col);
    panic_print_str("  EFLAGS: ", 0x4F, &row, &col);
    panic_print_hex(regs->eflags, 0x4E, &row, &col);
    panic_print_str("\n", 0x4F, &row, &col);

    panic_print_str("EAX: ", 0x4F, &row, &col);
    panic_print_hex(regs->eax, 0x4E, &row, &col);
    panic_print_str("  EBX:    ", 0x4F, &row, &col);
    panic_print_hex(regs->ebx, 0x4E, &row, &col);
    panic_print_str("  ECX:    ", 0x4F, &row, &col);
    panic_print_hex(regs->ecx, 0x4E, &row, &col);
    panic_print_str("  EDX: ", 0x4F, &row, &col);
    panic_print_hex(regs->edx, 0x4E, &row, &col);
    panic_print_str("\n", 0x4F, &row, &col);

    panic_print_str("ESI: ", 0x4F, &row, &col);
    panic_print_hex(regs->esi, 0x4E, &row, &col);
    panic_print_str("  EDI:    ", 0x4F, &row, &col);
    panic_print_hex(regs->edi, 0x4E, &row, &col);
    panic_print_str("  EBP:    ", 0x4F, &row, &col);
    panic_print_hex(regs->ebp, 0x4E, &row, &col);
    panic_print_str("  DS:  ", 0x4F, &row, &col);
    panic_print_hex(regs->ds, 0x4E, &row, &col);
    panic_print_str("\n\n", 0x4F, &row, &col);

    panic_print_str("System halted. Please reboot your machine.\n", 0x4C, &row, &col);

    /* Also write trace to serial COM1 */
    serial_write_str("\n*** KERNEL PANIC: ");
    serial_write_str(desc);
    serial_write_str(" (Vector ");
    serial_write_dec(regs->int_no);
    serial_write_str(", Error Code: ");
    serial_write_hex(regs->err_code);
    serial_write_str(") ***\nEIP: ");
    serial_write_hex(regs->eip);
    serial_write_str(" CS: ");
    serial_write_hex(regs->cs);
    serial_write_str(" EFLAGS: ");
    serial_write_hex(regs->eflags);
    serial_write_str("\nSystem halted.\n");

    /* Disable interrupts and halt CPU */
    cli();
    for (;;) {
        hlt();
    }
}

/*
 * Register a handler callback for a specific hardware IRQ (0 - 15)
 */
void register_irq_handler(uint8_t irq, void (*handler)(registers_t *regs)) {
    if (irq < IRQ_COUNT) {
        irq_handlers[irq] = handler;
    }
}

/*
 * Hardware IRQ Handler - dispatches to registered handler and acknowledges PIC
 */
void irq_handler(registers_t *regs) {
    if (regs->int_no >= 32 && regs->int_no < (32 + IRQ_COUNT)) {
        uint8_t irq = (uint8_t)(regs->int_no - 32);

        if (irq_handlers[irq] != NULL) {
            irq_handlers[irq](regs);
        }

        /* Send End-of-Interrupt signal to 8259 PIC */
        pic_send_eoi(irq);
    }
}

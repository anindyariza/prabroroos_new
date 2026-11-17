#pragma once
#include "../kernel/types.h"

/*
 * Structure containing registers as pushed by isr.asm and irq.asm
 */
typedef struct {
    uint32_t ds;
    uint32_t edi, esi, ebp, esp_dummy, ebx, edx, ecx, eax;
    uint32_t int_no, err_code;
    uint32_t eip, cs, eflags;
} registers_t;

typedef void (*irq_handler_t)(registers_t *regs);

void register_irq_handler(uint8_t irq, irq_handler_t handler);
void isr_handler(registers_t *regs);
void irq_handler(registers_t *regs);

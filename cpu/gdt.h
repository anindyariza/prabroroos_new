#pragma once
#include "../kernel/types.h"

#define GDT_NULL_SEG   0x00
#define GDT_CODE_SEG   0x08
#define GDT_DATA_SEG   0x10

typedef struct {
    uint16_t limit_low;
    uint16_t base_low;
    uint8_t  base_mid;
    uint8_t  access;
    uint8_t  gran;
    uint8_t  base_high;
} __attribute__((packed)) gdt_entry_t;

typedef struct {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed)) gdt_ptr_t;

void gdt_init(void);
extern void gdt_flush(uint32_t);

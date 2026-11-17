#include "gdt.h"

#define GDT_NUM_ENTRIES 3

static gdt_entry_t gdt_entries[GDT_NUM_ENTRIES];
static gdt_ptr_t   gdt_ptr;

static void gdt_set_entry(int num, uint32_t base, uint32_t limit, uint8_t access, uint8_t gran) {
    gdt_entries[num].base_low    = (uint16_t)(base & 0xFFFF);
    gdt_entries[num].base_mid    = (uint8_t)((base >> 16) & 0xFF);
    gdt_entries[num].base_high   = (uint8_t)((base >> 24) & 0xFF);

    gdt_entries[num].limit_low   = (uint16_t)(limit & 0xFFFF);
    gdt_entries[num].gran        = (uint8_t)((limit >> 16) & 0x0F);

    gdt_entries[num].gran       |= (uint8_t)(gran & 0xF0);
    gdt_entries[num].access      = access;
}

void gdt_init(void) {
    gdt_ptr.limit = (uint16_t)(sizeof(gdt_entry_t) * GDT_NUM_ENTRIES - 1);
    gdt_ptr.base  = (uint32_t)&gdt_entries;

    /* Entry 0: Null descriptor */
    gdt_set_entry(0, 0, 0, 0, 0);

    /* Entry 1: Kernel Code Segment
     * Base: 0x00000000, Limit: 0xFFFFF (spanning 4 GB with 4 KB granularity)
     * Access: 0x9A (Present, Ring 0, Executable, Readable)
     * Granularity: 0xCF (4 KB blocks, 32-bit protected mode)
     */
    gdt_set_entry(1, 0, 0xFFFFF, 0x9A, 0xCF);

    /* Entry 2: Kernel Data Segment
     * Base: 0x00000000, Limit: 0xFFFFF (spanning 4 GB with 4 KB granularity)
     * Access: 0x92 (Present, Ring 0, Writable)
     * Granularity: 0xCF (4 KB blocks, 32-bit protected mode)
     */
    gdt_set_entry(2, 0, 0xFFFFF, 0x92, 0xCF);

    /* Flush and reload segment registers */
    gdt_flush((uint32_t)&gdt_ptr);
}

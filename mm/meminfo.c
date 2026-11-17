/*
 * PrabroroOS — mm/meminfo.c
 */
#include "meminfo.h"
#include "kmalloc.h"
#include "../drivers/vga.h"
#include "../kernel/types.h"

static void mi_print_uint(uint32_t v) {
    if (v == 0) { vga_putchar('0'); return; }
    char buf[12]; int i = 0;
    while (v) { buf[i++] = '0' + (v%10); v/=10; }
    while (i--) vga_putchar(buf[i]);
}

void meminfo_print(void) {
    size_t total  = heap_total();
    size_t used   = heap_used();
    size_t free_b = heap_free();
    uint32_t pct  = total ? (uint32_t)((used * 100) / total) : 0;

    vga_println_colored("=== Memory Info (Heap Allocator) ===", VGA_LIGHT_CYAN, VGA_BLACK);

    vga_print("  Total : "); mi_print_uint((uint32_t)(total/1024));
    vga_println(" KB");

    vga_print_colored("  Used  : ", VGA_LIGHT_RED, VGA_BLACK);
    mi_print_uint((uint32_t)(used/1024));
    vga_print(" KB ("); mi_print_uint(pct); vga_println("%)");

    vga_print_colored("  Free  : ", VGA_LIGHT_GREEN, VGA_BLACK);
    mi_print_uint((uint32_t)(free_b/1024));
    vga_print(" KB ("); mi_print_uint(100-pct); vga_println("%)");

    vga_print("  Bar   : [");
    uint32_t filled = (pct * 20) / 100;
    for (uint32_t i = 0; i < 20; i++) {
        if (i < filled) vga_print_colored("#", VGA_LIGHT_RED, VGA_BLACK);
        else            vga_print_colored(".", VGA_DARK_GREY, VGA_BLACK);
    }
    vga_print("] "); mi_print_uint(pct); vga_println("%");
}

void meminfo_print_summary(void) {
    size_t used = heap_used();
    size_t total = heap_total();
    uint32_t pct = total ? (uint32_t)((used*100)/total) : 0;
    vga_print("Mem: ");
    mi_print_uint((uint32_t)(used/1024));
    vga_print("/");
    mi_print_uint((uint32_t)(total/1024));
    vga_print(" KB (");
    mi_print_uint(pct);
    vga_print("%)");
}

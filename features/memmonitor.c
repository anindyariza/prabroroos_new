/*
 * PrabroroOS — Memory Usage Monitor
 */
#include "memmonitor.h"
#include "../mm/kmalloc.h"
#include "../drivers/vga.h"
#include "../kernel/types.h"

static void mm_print_uint(uint32_t v) {
    if (v == 0) { vga_putchar('0'); return; }
    char buf[12]; int i = 0;
    while (v) { buf[i++] = '0' + (v % 10); v /= 10; }
    while (i--) vga_putchar(buf[i]);
}

void memmonitor_print(void) {
    size_t total  = heap_total();
    size_t used   = heap_used();
    size_t free_b = heap_free();

    uint32_t pct_used = (total > 0) ? (uint32_t)((used * 100) / total) : 0;
    uint32_t pct_free = 100 - pct_used;

    vga_println_colored("╔══════════════════════════════════════╗", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_println_colored("║      PrabroroOS Memory Monitor       ║", VGA_LIGHT_CYAN, VGA_BLACK);
    vga_println_colored("╠══════════════════════════════════════╣", VGA_LIGHT_CYAN, VGA_BLACK);

    vga_print_colored("║ Total Heap : ", VGA_WHITE, VGA_BLACK);
    mm_print_uint((uint32_t)(total / 1024));
    vga_print(" KB (");
    mm_print_uint((uint32_t)total);
    vga_println(" bytes)             ║");

    vga_print_colored("║ Used       : ", VGA_LIGHT_RED, VGA_BLACK);
    mm_print_uint((uint32_t)(used / 1024));
    vga_print(" KB  (");
    mm_print_uint(pct_used);
    vga_println("%)                      ║");

    vga_print_colored("║ Free       : ", VGA_LIGHT_GREEN, VGA_BLACK);
    mm_print_uint((uint32_t)(free_b / 1024));
    vga_print(" KB  (");
    mm_print_uint(pct_free);
    vga_println("%)                      ║");

    vga_println_colored("╠══════════════════════════════════════╣", VGA_LIGHT_CYAN, VGA_BLACK);

    /* Progress bar — 32 chars wide */
    vga_print_colored("║ Usage: [", VGA_WHITE, VGA_BLACK);
    uint32_t filled = (pct_used * 32) / 100;
    for (uint32_t i = 0; i < 32; i++) {
        if (i < filled)
            vga_print_colored("#", VGA_LIGHT_RED, VGA_BLACK);
        else
            vga_print_colored(".", VGA_DARK_GREY, VGA_BLACK);
    }
    vga_print("] ");
    mm_print_uint(pct_used);
    vga_println("%  ║");

    vga_println_colored("╚══════════════════════════════════════╝", VGA_LIGHT_CYAN, VGA_BLACK);
}

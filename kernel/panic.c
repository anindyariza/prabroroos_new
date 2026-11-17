/*
 * PrabroroOS — Kernel Panic Handler
 */
#include "panic.h"
#include "../drivers/vga.h"

void kernel_panic(const char *msg) {
    /* Disable interrupts immediately */
    __asm__ volatile("cli");

    vga_set_color(VGA_LIGHT_RED, VGA_BLACK);
    vga_println("\n\n!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    vga_println("!!!         KERNEL PANIC                 !!!");
    vga_println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    vga_print("!!! ");
    vga_print(msg);
    vga_println("");
    vga_println("!!! System halted. Reset to restart.");
    vga_println("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
    vga_set_color(VGA_WHITE, VGA_BLACK);

    for (;;) {
        __asm__ volatile("hlt");
    }
}

void kernel_assert(int condition, const char *msg) {
    if (!condition) {
        kernel_panic(msg);
    }
}

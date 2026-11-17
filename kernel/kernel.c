/*
 * PrabroroOS — kernel/kernel.c
 * Main kernel entry point, called from boot/boot.asm after BSS zeroed.
 */

#include "../cpu/gdt.h"
#include "../cpu/idt.h"
#include "../cpu/pic.h"
#include "../drivers/vga.h"
#include "../drivers/keyboard.h"
#include "../drivers/pit.h"
#include "../mm/kmalloc.h"
#include "../proc/process.h"
#include "../proc/scheduler.h"
#include "../fs/ramdisk.h"
#include "../fs/vfs.h"
#include "../security/acl.h"
#include "../features/taskqueue.h"
#include "../shell/shell.h"
#include "panic.h"
#include "types.h"

/* Heap base from linker script */
extern uint8_t _kernel_end;

/* PIT tick callback for task queue (1 second = 100 ticks at 100Hz) */
static uint32_t tq_tick_ctr = 0;
static void taskqueue_pit_cb(uint32_t ticks) {
    (void)ticks;
    tq_tick_ctr++;
    if (tq_tick_ctr >= 100) {
        tq_tick_ctr = 0;
        taskqueue_tick();
    }
}

/* ------------------------------------------------------------------ */
/* ASCII Logo                                                           */
/* ------------------------------------------------------------------ */
static void print_logo(void) {
    vga_set_color(VGA_YELLOW, VGA_BLACK);
    vga_println("  ____           _                     ___  ____  ");
    vga_println(" |  _ \\ _ __ __ _| |__  _ __ ___  _ __|/ _ \\/ ___| ");
    vga_println(" | |_) | '__/ _` | '_ \\| '__/ _ \\| '__| | | \\___ \\ ");
    vga_println(" |  __/| | | (_| | |_) | | | (_) | |  | |_| |___) |");
    vga_println(" |_|   |_|  \\__,_|_.__/|_|  \\___/|_|   \\___/|____/ ");
    vga_println("");

    vga_set_color(VGA_LIGHT_CYAN, VGA_BLACK);
    vga_println("   ======================================================");
    vga_println("   ||     PrabroroOS v1.0  —  x86 32-bit Kernel        ||");
    vga_println("   ||      Real OS • QEMU • Multiboot • GRUB            ||");
    vga_println("   ======================================================");
    vga_println("");

    vga_set_color(VGA_LIGHT_GREEN, VGA_BLACK);
    vga_println("   [BOOT] GDT        ... OK");
    vga_println("   [BOOT] IDT        ... OK");
    vga_println("   [BOOT] PIC 8259A  ... OK");
    vga_println("   [BOOT] PIT 100Hz  ... OK");
    vga_println("   [BOOT] Heap       ... OK (4 MB)");
    vga_println("   [BOOT] Scheduler  ... OK");
    vga_println("   [BOOT] VFS/RAMDsk ... OK");
    vga_println("   [BOOT] ACL        ... OK");
    vga_println("   [BOOT] TaskQueue  ... OK");
    vga_println("");

    vga_set_color(VGA_WHITE, VGA_BLACK);
    vga_println("   Ketik 'help' untuk melihat semua perintah.");
    vga_println("");
    vga_set_color(VGA_WHITE, VGA_BLACK);
}

/* ------------------------------------------------------------------ */
/* Idle thread: runs when nothing else is ready                         */
/* ------------------------------------------------------------------ */
static void idle_thread_fn(void) {
    for (;;) {
        __asm__ volatile("hlt");
    }
}

/* ------------------------------------------------------------------ */
/* kernel_main — called from boot.asm                                  */
/* ------------------------------------------------------------------ */
void kernel_main(uint32_t magic, uint32_t mb_info) {
    (void)magic;
    (void)mb_info;

    /* 1. CPU infrastructure */
    gdt_init();
    idt_init();
    pic_init();
    pit_init(100);  /* 100 Hz system tick */

    /* 2. Heap: start right after kernel binary, 4 MB */
    uintptr_t heap_start = (uintptr_t)&_kernel_end;
    /* Align to 4KB */
    heap_start = (heap_start + 0xFFF) & ~0xFFFU;
    heap_init(heap_start, 4 * 1024 * 1024);

    /* 3. VGA — now safe to print */
    vga_init();
    vga_clear();

    /* 4. Display boot logo */
    print_logo();

    /* 5. Register PIT callback for task queue countdown */
    pit_register_callback(taskqueue_pit_cb);

    /* 6. Subsystems */
    process_init();
    scheduler_init();
    ramdisk_init();
    vfs_init();
    acl_init();
    taskqueue_init();
    keyboard_init();

    /* 7. Enable interrupts */
    __asm__ volatile("sti");

    /* 8. Create idle thread */
    thread_create("idle", idle_thread_fn, 0);

    /* 9. Shell (runs in main kernel context, not in a separate thread) */
    shell_init();
    shell_run();

    /* Should never reach here */
    for (;;) {
        __asm__ volatile("hlt");
    }
}

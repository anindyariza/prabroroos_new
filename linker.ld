/*
 * PrabroroOS Linker Script
 * Target: x86 32-bit ELF (Multiboot compliant)
 */

ENTRY(_start)

SECTIONS
{
    /* Load kernel at 1MB physical address */
    . = 0x00100000;

    _kernel_start = .;

    /* Multiboot header must be in the first 8KB of the kernel image */
    .multiboot ALIGN(4) :
    {
        *(.multiboot)
    }

    /* Kernel code */
    .text ALIGN(4096) :
    {
        *(.text*)
    }

    /* Read-only data */
    .rodata ALIGN(4096) :
    {
        *(.rodata*)
    }

    /* Initialized read-write data */
    .data ALIGN(4096) :
    {
        *(.data*)
    }

    /* Uninitialized read-write data and kernel stack */
    .bss ALIGN(4096) :
    {
        _bss_start = .;
        *(COMMON)
        *(.bss*)
        _bss_end = .;
    }

    _kernel_end = .;
}

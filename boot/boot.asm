; PrabroroOS - Multiboot 1 Kernel Entry Point
[BITS 32]

; Multiboot 1 header constants
MBALIGN     equ 1 << 0                   ; Align loaded modules on page boundaries
MEMINFO     equ 1 << 1                   ; Provide memory map
FLAGS       equ MBALIGN | MEMINFO        ; Multiboot flags (0x03)
MAGIC       equ 0x1BADB002               ; Multiboot magic number
CHECKSUM    equ -(MAGIC + FLAGS)         ; Checksum: MAGIC + FLAGS + CHECKSUM == 0

section .multiboot
align 4
    dd MAGIC
    dd FLAGS
    dd CHECKSUM

section .bss
align 16
stack_bottom:
    resb 16384                          ; Allocate 16 KB kernel stack
stack_top:

section .text
global _start
extern _bss_start
extern _bss_end
extern kernel_main

_start:
    ; Initialize the stack pointer to stack top
    mov esp, stack_top

    ; Preserve Multiboot registers passed by the bootloader:
    ; EAX = Multiboot magic value (0x2BADB002)
    ; EBX = Physical address of Multiboot info structure
    mov edi, eax
    mov esi, ebx

    ; Zero out the BSS section
    mov edx, _bss_start
    mov ecx, _bss_end
    sub ecx, edx
    jz .bss_done
    xor eax, eax

.zero_bss_loop:
    mov byte [edx], al
    inc edx
    dec ecx
    jnz .zero_bss_loop

.bss_done:
    ; Push parameters for kernel_main(uint32_t magic, uint32_t mb_info_addr)
    ; Follows standard cdecl calling convention
    push esi                            ; Argument 2: Multiboot info structure pointer (EBX)
    push edi                            ; Argument 1: Multiboot magic value (EAX)
    call kernel_main

    ; Infinite loop with interrupts disabled if kernel_main exits
.halt:
    cli
    hlt
    jmp .halt

; PrabroroOS - GDT Flush Routine
[BITS 32]

section .text
global gdt_flush

gdt_flush:
    mov eax, [esp+4]    ; Retrieve GDT pointer passed as argument
    lgdt [eax]          ; Load GDT

    mov ax, 0x10        ; 0x10 is the kernel data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Far jump to flush instruction pipeline and update CS to 0x08 (kernel code segment)
    jmp 0x08:.flush

.flush:
    ret

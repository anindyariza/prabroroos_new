; PrabroroOS - IDT Load Routine
[BITS 32]

section .text
global idt_load

idt_load:
    mov eax, [esp+4]    ; IDT pointer structure argument
    lidt [eax]          ; Load IDTR with IDT pointer
    ret

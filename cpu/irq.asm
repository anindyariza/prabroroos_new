; PrabroroOS - Hardware IRQ Stubs (IRQ 0 - 15, Vectors 32 - 47)
[BITS 32]

section .text
extern irq_handler

; Common IRQ stub: saves CPU state, prepares stack, calls C irq_handler, and restores state
; Compatible with registers_t layout in isr_handler.c
irq_common_stub:
    pusha                       ; Pushes EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX

    mov ax, ds                  ; Lower 16-bits of EAX = DS
    push eax                    ; Save original data segment descriptor

    mov ax, 0x10                ; Load kernel data segment descriptor (0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp                    ; Push pointer to registers_t struct
    call irq_handler            ; Call C hardware interrupt dispatcher
    add esp, 4                  ; Clean up pushed pointer

    pop eax                     ; Restore original data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                        ; Restores EDI, ESI, EBP, ESP (skipped), EBX, EDX, ECX, EAX
    add esp, 8                  ; Clean up pushed int_no and dummy err_code
    iret                        ; Return from interrupt

; Macro for IRQ stubs
; %1: IRQ number (0 - 15)
; %2: Interrupt vector number (32 - 47)
%macro IRQ 2
global irq%1
irq%1:
    push dword 0                ; Push dummy error code
    push dword %2               ; Push interrupt vector number
    jmp irq_common_stub
%endmacro

; Hardware IRQ Stubs
IRQ  0, 32                      ; IRQ0: Programmable Interval Timer
IRQ  1, 33                      ; IRQ1: Keyboard
IRQ  2, 34                      ; IRQ2: Cascade (used internally by PIC2)
IRQ  3, 35                      ; IRQ3: COM2 (Serial Port 2)
IRQ  4, 36                      ; IRQ4: COM1 (Serial Port 1)
IRQ  5, 37                      ; IRQ5: LPT2 (Parallel Port 2)
IRQ  6, 38                      ; IRQ6: Floppy Disk
IRQ  7, 39                      ; IRQ7: LPT1 (Parallel Port 1 / Spurious)
IRQ  8, 40                      ; IRQ8: CMOS Real-Time Clock
IRQ  9, 41                      ; IRQ9: Free for peripherals / SCSI / NIC
IRQ 10, 42                      ; IRQ10: Free for peripherals / SCSI / NIC
IRQ 11, 43                      ; IRQ11: Free for peripherals / SCSI / NIC
IRQ 12, 44                      ; IRQ12: PS/2 Mouse
IRQ 13, 45                      ; IRQ13: FPU / Coprocessor
IRQ 14, 46                      ; IRQ14: Primary ATA Hard Disk
IRQ 15, 47                      ; IRQ15: Secondary ATA Hard Disk

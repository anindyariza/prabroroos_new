; PrabroroOS - CPU Exception ISR Stubs (Vectors 0 - 31)
[BITS 32]

section .text
extern isr_handler

; Common ISR stub: saves CPU state, prepares stack, calls C isr_handler, and restores state
;
; Stack layout for registers_t:
;   [esp + 0]  : ds
;   [esp + 4]  : edi
;   [esp + 8]  : esi
;   [esp + 12] : ebp
;   [esp + 16] : esp_dummy (from pusha)
;   [esp + 20] : ebx
;   [esp + 24] : edx
;   [esp + 28] : ecx
;   [esp + 32] : eax
;   [esp + 36] : int_no
;   [esp + 40] : err_code
;   [esp + 44] : eip (pushed by CPU)
;   [esp + 48] : cs  (pushed by CPU)
;   [esp + 52] : eflags (pushed by CPU)

isr_common_stub:
    pusha                       ; Pushes EDI, ESI, EBP, ESP, EBX, EDX, ECX, EAX

    mov ax, ds                  ; Lower 16-bits of EAX = DS
    push eax                    ; Save original data segment descriptor

    mov ax, 0x10                ; Load kernel data segment descriptor (0x10)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    push esp                    ; Push pointer to registers_t struct
    call isr_handler            ; Call C exception dispatcher
    add esp, 4                  ; Clean up pushed pointer

    pop eax                     ; Restore original data segment descriptor
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    popa                        ; Restores EDI, ESI, EBP, ESP (skipped), EBX, EDX, ECX, EAX
    add esp, 8                  ; Clean up pushed int_no and err_code
    iret                        ; Return from interrupt

; Macro for exceptions that DO NOT push an error code (dummy 0 pushed)
%macro ISR_NOERRCODE 1
global isr%1
isr%1:
    push dword 0                ; Push dummy error code
    push dword %1               ; Push interrupt vector number
    jmp isr_common_stub
%endmacro

; Macro for exceptions that DO push an error code automatically by CPU
%macro ISR_ERRCODE 1
global isr%1
isr%1:
    ; CPU automatically pushed error code
    push dword %1               ; Push interrupt vector number
    jmp isr_common_stub
%endmacro

; Exception Stubs (0 - 31)
; Vectors 8, 10, 11, 12, 13, 14, 17 push real error codes; others push dummy 0
ISR_NOERRCODE 0                 ; 0: Divide By Zero
ISR_NOERRCODE 1                 ; 1: Debug
ISR_NOERRCODE 2                 ; 2: Non-Maskable Interrupt
ISR_NOERRCODE 3                 ; 3: Breakpoint
ISR_NOERRCODE 4                 ; 4: Into Detected Overflow
ISR_NOERRCODE 5                 ; 5: Out of Bounds
ISR_NOERRCODE 6                 ; 6: Invalid Opcode
ISR_NOERRCODE 7                 ; 7: No Coprocessor
ISR_ERRCODE   8                 ; 8: Double Fault (pushes error code)
ISR_NOERRCODE 9                 ; 9: Coprocessor Segment Overrun
ISR_ERRCODE   10                ; 10: Bad TSS (pushes error code)
ISR_ERRCODE   11                ; 11: Segment Not Present (pushes error code)
ISR_ERRCODE   12                ; 12: Stack Fault (pushes error code)
ISR_ERRCODE   13                ; 13: General Protection Fault (pushes error code)
ISR_ERRCODE   14                ; 14: Page Fault (pushes error code)
ISR_NOERRCODE 15                ; 15: Unknown Interrupt
ISR_NOERRCODE 16                ; 16: Coprocessor Fault
ISR_ERRCODE   17                ; 17: Alignment Check (pushes error code)
ISR_NOERRCODE 18                ; 18: Machine Check
ISR_NOERRCODE 19                ; 19: SIMD Floating-Point Exception
ISR_NOERRCODE 20                ; 20: Virtualization Exception
ISR_NOERRCODE 21                ; 21: Control Protection Exception
ISR_NOERRCODE 22                ; 22: Reserved
ISR_NOERRCODE 23                ; 23: Reserved
ISR_NOERRCODE 24                ; 24: Reserved
ISR_NOERRCODE 25                ; 25: Reserved
ISR_NOERRCODE 26                ; 26: Reserved
ISR_NOERRCODE 27                ; 27: Reserved
ISR_NOERRCODE 28                ; 28: Hypervisor Injection Exception
ISR_NOERRCODE 29                ; 29: VMM Communication Exception
ISR_NOERRCODE 30                ; 30: Security Exception
ISR_NOERRCODE 31                ; 31: Reserved

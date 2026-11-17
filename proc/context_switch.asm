[bits 32]
section .text

; void context_switch(uint32_t *old_esp_ptr, uint32_t new_esp)
; [esp+4] = pointer to old task's esp field (uint32_t*)
; [esp+8] = new task's esp value
global context_switch
context_switch:
    ; Save callee-saved regs of current task
    push ebp
    push ebx
    push esi
    push edi
    
    ; Save current ESP to *old_esp_ptr
    mov eax, [esp + 20]   ; old_esp_ptr (after 4 pushes + return addr = 5*4=20)
    mov [eax], esp        ; *old_esp_ptr = esp
    
    ; Switch to new stack
    mov eax, [esp + 24]   ; new_esp
    mov esp, eax
    
    ; Restore callee-saved regs of new task
    pop edi
    pop esi
    pop ebx
    pop ebp
    
    ret

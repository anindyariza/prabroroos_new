#pragma once
#include "../proc/process.h"

void scheduler_init(void);
void scheduler_add(tcb_t *thread);
void scheduler_remove(tcb_t *thread);
void scheduler_tick(void);   // called from PIT IRQ handler every tick
void scheduler_yield(void);  // voluntarily give up CPU
tcb_t *scheduler_get_current(void);

// Called from context_switch.asm:
extern void context_switch(uint32_t *old_esp_ptr, uint32_t new_esp);

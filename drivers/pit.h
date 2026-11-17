/*
 * PrabroroOS — drivers/pit.h
 */
#pragma once
#include "../kernel/types.h"

typedef void (*tick_callback_t)(uint32_t ticks);

void     pit_init(uint32_t frequency_hz);
uint32_t pit_get_ticks(void);
void     pit_irq_handler(void *regs);
void     pit_register_callback(tick_callback_t cb);
